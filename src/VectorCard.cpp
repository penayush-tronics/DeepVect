#include "VectorCard.h"

#include "AppModel.h"
#include "Theme.h"
#include "VectorExpression.h"
#include "VectorTextEdit.h"

#include <QApplication>
#include <QCheckBox>
#include <QEvent>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSignalBlocker>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace {

bool isEditing(const QWidget* widget) {
    const QWidget* focus = QApplication::focusWidget();
    return focus != nullptr && (focus == widget || widget->isAncestorOf(focus));
}

QString angleLabel(double component, double length) {
    if (length < 1e-12) {
        return QStringLiteral("undefined");
    }
    const double cosine = std::clamp(component / length, -1.0, 1.0);
    const double degrees = std::acos(cosine) * 180.0 / std::numbers::pi;
    return QString::number(degrees, 'f', 1) + QStringLiteral("\u00B0");
}

} // namespace

VectorCard::VectorCard(AppModel* model, int vectorId, QWidget* parent)
    : QFrame(parent)
    , m_model(model)
    , m_id(vectorId) {
    setObjectName(QStringLiteral("Card"));
    setFrameShape(QFrame::NoFrame);
    setProperty("selected", false);
    setProperty("error", false);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 9, 10, 10);
    layout->setSpacing(6);

    auto* header = new QHBoxLayout;
    header->setSpacing(6);

    m_colorDot = new QLabel(this);
    m_colorDot->setFixedSize(12, 12);
    m_colorDot->setToolTip(QStringLiteral("Vector color"));

    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setObjectName(QStringLiteral("InlineName"));
    m_nameEdit->setMinimumWidth(70);
    m_nameEdit->setMaximumWidth(150);
    m_nameEdit->setToolTip(QStringLiteral("Vector name"));

    m_visibleButton = new QCheckBox(this);
    m_visibleButton->setToolTip(QStringLiteral("Show on graph"));

    m_deleteButton = new QToolButton(this);
    m_deleteButton->setText(QStringLiteral("\u00D7"));
    m_deleteButton->setToolTip(QStringLiteral("Remove vector"));

    header->addWidget(m_colorDot);
    header->addWidget(m_nameEdit, 1);
    header->addWidget(m_visibleButton);
    header->addWidget(m_deleteButton);
    layout->addLayout(header);

    m_vectorEdit = new VectorTextEdit(this);
    layout->addWidget(m_vectorEdit);

    m_errorLabel = new QLabel(this);
    m_errorLabel->setObjectName(QStringLiteral("ErrorLabel"));
    m_errorLabel->setWordWrap(true);
    m_errorLabel->hide();
    layout->addWidget(m_errorLabel);

    m_statsWidget = new QWidget(this);
    auto* statsLayout = new QVBoxLayout(m_statsWidget);
    statsLayout->setContentsMargins(0, 4, 0, 0);
    statsLayout->setSpacing(4);

    auto* statsTitle = new QLabel(QStringLiteral("STATISTICS"), m_statsWidget);
    statsTitle->setObjectName(QStringLiteral("SectionTitle"));
    statsLayout->addWidget(statsTitle);

    auto* statsForm = new QFormLayout;
    statsForm->setContentsMargins(0, 0, 0, 0);
    statsForm->setHorizontalSpacing(12);
    statsForm->setVerticalSpacing(4);

    m_magnitudeLabel = new QLabel(m_statsWidget);
    m_magnitudeLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    statsForm->addRow(QStringLiteral("Magnitude"), m_magnitudeLabel);

    auto* unitContainer = new QWidget(m_statsWidget);
    auto* unitLayout = new QVBoxLayout(unitContainer);
    unitLayout->setContentsMargins(0, 0, 0, 0);
    unitLayout->setSpacing(2);
    auto* unitTitle = new QLabel(QStringLiteral("Unit vector"), unitContainer);
    unitTitle->setObjectName(QStringLiteral("Hint"));
    auto* unitRow = new QHBoxLayout;
    unitRow->setContentsMargins(0, 0, 0, 0);
    unitRow->setSpacing(4);
    m_unitLabel = new QLabel(unitContainer);
    m_unitLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_unitLabel->setWordWrap(true);
    m_unitUndefined = new QLabel(QStringLiteral("undefined"), unitContainer);
    m_unitUndefined->setObjectName(QStringLiteral("Hint"));
    unitRow->addWidget(m_unitLabel, 1);
    unitRow->addWidget(m_unitUndefined);
    unitLayout->addWidget(unitTitle);
    unitLayout->addLayout(unitRow);
    statsForm->addRow(unitContainer);

    m_directionLabel = new QLabel(m_statsWidget);
    m_directionLabel->setWordWrap(true);
    m_directionLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    statsForm->addRow(QStringLiteral("Direction"), m_directionLabel);

    statsLayout->addLayout(statsForm);
    layout->addWidget(m_statsWidget);
    m_statsWidget->hide();

    connect(m_model, &AppModel::vectorUpdated, this, [this](int id) {
        if (id == m_id) {
            refresh();
        }
    });
    connect(m_model, &AppModel::settingsChanged, this, [this]() {
        refresh();
    });

    connect(m_nameEdit, &QLineEdit::editingFinished, this, [this]() {
        const VectorEntry* entry = m_model->vectorById(m_id);
        if (entry == nullptr || m_nameEdit->text() == entry->name) {
            return;
        }

        QString error;
        if (!m_model->renameVector(m_id, m_nameEdit->text(), &error)) {
            showError(error);
            const QSignalBlocker blocker(m_nameEdit);
            m_nameEdit->setText(entry->name);
        } else {
            clearError();
        }
    });

    connect(m_vectorEdit, &VectorTextEdit::valueEdited, this, [this](const Vector3& value) {
        emit activated(m_id);
        m_model->updateVectorValue(m_id, value);
    });
    connect(m_vectorEdit, &VectorTextEdit::validityChanged, this, [this](bool valid, const QString& error) {
        if (valid) {
            clearError();
        } else {
            showError(error);
        }
    });

    connect(m_visibleButton, &QCheckBox::toggled, this, [this](bool checked) {
        m_model->setVisible(m_id, checked);
    });
    connect(m_deleteButton, &QToolButton::clicked, this, [this]() {
        m_model->removeVector(m_id);
    });

    installActivationFilter(this);

    m_vectorEdit->setFormat(m_model->inputFormat());
    refresh();
}

int VectorCard::vectorId() const {
    return m_id;
}

void VectorCard::refresh() {
    const VectorEntry* entry = m_model->vectorById(m_id);
    if (entry == nullptr) {
        return;
    }

    const ThemeColors colors = Theme::colors(m_model->themeMode());
    QColor color = colors.accent;
    if (!colors.vectorColors.isEmpty()) {
        color = colors.vectorColors.at(entry->colorIndex % colors.vectorColors.size());
    }
    m_colorDot->setStyleSheet(
        QStringLiteral("background-color: %1; border-radius: 6px;").arg(color.name()));

    if (!m_nameEdit->hasFocus()) {
        const QSignalBlocker blocker(m_nameEdit);
        m_nameEdit->setText(entry->name);
    }

    {
        const QSignalBlocker blocker(m_visibleButton);
        m_visibleButton->setChecked(entry->visible);
    }

    const bool editingVector = isEditing(m_vectorEdit);
    if (!editingVector && (!m_vectorEdit->isValid() || m_vectorEdit->value() != entry->value)) {
        m_vectorEdit->setVector(entry->value);
    }

    if (m_vectorEdit->format() != m_model->inputFormat()) {
        m_vectorEdit->setFormat(m_model->inputFormat());
        m_vectorEdit->setVector(entry->value);
    }

    updateStats(*entry);
}

void VectorCard::setSelected(bool selected) {
    setProperty("selected", selected);
    style()->unpolish(this);
    style()->polish(this);
    m_statsWidget->setVisible(selected);
    if (!selected) {
        refresh();
    }
}

void VectorCard::setInputFormat(InputFormat format) {
    const VectorEntry* entry = m_model->vectorById(m_id);
    if (entry == nullptr) {
        return;
    }

    m_vectorEdit->setFormat(format);
    m_vectorEdit->setVector(entry->value);
    updateStats(*entry);
    clearError();
}

void VectorCard::updateStats(const VectorEntry& entry) {
    const double length = entry.value.length();
    m_magnitudeLabel->setText(VectorExpression::formatNumber(length));

    const bool isZero = length < 1e-12;
    m_unitUndefined->setVisible(isZero);
    m_unitLabel->setVisible(!isZero);
    if (!isZero) {
        m_unitLabel->setText(VectorExpression::formatUnitFraction(
            entry.value, m_model->inputFormat() == InputFormat::Column));
    }

    if (isZero) {
        m_directionLabel->setText(QStringLiteral("undefined"));
    } else {
        const QString alpha = angleLabel(entry.value.x, length);
        const QString beta = angleLabel(entry.value.y, length);
        const QString gamma = angleLabel(entry.value.z, length);
        m_directionLabel->setText(
            QStringLiteral("\u03B1 = %1, \u03B2 = %2, \u03B3 = %3")
                .arg(alpha, beta, gamma));
    }
}

void VectorCard::clearError() {
    m_errorLabel->clear();
    m_errorLabel->hide();
    setProperty("error", false);
    style()->unpolish(this);
    style()->polish(this);
}

void VectorCard::showError(const QString& error) {
    m_errorLabel->setText(error);
    m_errorLabel->show();
    setProperty("error", true);
    style()->unpolish(this);
    style()->polish(this);
}

bool VectorCard::eventFilter(QObject* watched, QEvent* event) {
    Q_UNUSED(watched);

    if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::FocusIn) {
        emit activated(m_id);
    }

    return QFrame::eventFilter(watched, event);
}

void VectorCard::installActivationFilter(QWidget* widget) {
    widget->installEventFilter(this);
    const auto children = widget->children();
    for (QObject* child : children) {
        if (auto* childWidget = qobject_cast<QWidget*>(child)) {
            installActivationFilter(childWidget);
        }
    }
}
