#include "VectorTextEdit.h"

#include "VectorExpression.h"

#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSignalBlocker>
#include <QVBoxLayout>

#include <cmath>

namespace {

QLineEdit* createColumnEdit(const QString& placeholder) {
    auto* edit = new QLineEdit;
    edit->setPlaceholderText(placeholder);
    edit->setAlignment(Qt::AlignRight);
    edit->setMaximumWidth(110);
    edit->setMinimumWidth(64);
    return edit;
}

} // namespace

VectorTextEdit::VectorTextEdit(QWidget* parent)
    : QWidget(parent) {
    m_stack = new QVBoxLayout(this);
    m_stack->setContentsMargins(0, 0, 0, 0);
    m_stack->setSpacing(0);

    m_ijkPage = new QWidget(this);
    auto* ijkLayout = new QHBoxLayout(m_ijkPage);
    ijkLayout->setContentsMargins(0, 0, 0, 0);
    m_ijkEdit = new QLineEdit(m_ijkPage);
    m_ijkEdit->setPlaceholderText(QStringLiteral("3i + 2j + k"));
    m_ijkEdit->setClearButtonEnabled(false);
    ijkLayout->addWidget(m_ijkEdit);

    m_columnPage = new QWidget(this);
    auto* columnOuter = new QHBoxLayout(m_columnPage);
    columnOuter->setContentsMargins(0, 0, 0, 0);
    columnOuter->setSpacing(4);

    auto* leftBracket = new QLabel(QStringLiteral("["), m_columnPage);
    auto* rightBracket = new QLabel(QStringLiteral("]"), m_columnPage);
    QFont bracketFont = leftBracket->font();
    bracketFont.setPointSize(bracketFont.pointSize() + 10);
    bracketFont.setBold(true);
    leftBracket->setFont(bracketFont);
    rightBracket->setFont(bracketFont);
    leftBracket->setAlignment(Qt::AlignVCenter);
    rightBracket->setAlignment(Qt::AlignVCenter);

    auto* fieldsLayout = new QVBoxLayout;
    fieldsLayout->setContentsMargins(0, 0, 0, 0);
    fieldsLayout->setSpacing(2);
    m_xEdit = createColumnEdit(QStringLiteral("x"));
    m_yEdit = createColumnEdit(QStringLiteral("y"));
    m_zEdit = createColumnEdit(QStringLiteral("z"));
    fieldsLayout->addWidget(m_xEdit);
    fieldsLayout->addWidget(m_yEdit);
    fieldsLayout->addWidget(m_zEdit);

    columnOuter->addWidget(leftBracket);
    columnOuter->addLayout(fieldsLayout);
    columnOuter->addWidget(rightBracket);
    columnOuter->addStretch();

    m_stack->addWidget(m_ijkPage);
    m_stack->addWidget(m_columnPage);
    m_columnPage->hide();

    connect(m_ijkEdit, &QLineEdit::textChanged, this, &VectorTextEdit::handleIjkChanged);
    connect(m_xEdit, &QLineEdit::textChanged, this, &VectorTextEdit::handleColumnChanged);
    connect(m_yEdit, &QLineEdit::textChanged, this, &VectorTextEdit::handleColumnChanged);
    connect(m_zEdit, &QLineEdit::textChanged, this, &VectorTextEdit::handleColumnChanged);

    m_value = Vector3(3.0, 2.0, 1.0);
    populate();
    updateFocusProxy();
}

Vector3 VectorTextEdit::value() const {
    return m_value;
}

InputFormat VectorTextEdit::format() const {
    return m_format;
}

bool VectorTextEdit::isValid() const {
    return m_valid;
}

QString VectorTextEdit::errorText() const {
    return m_error;
}

bool VectorTextEdit::isReadOnly() const {
    return m_readOnly;
}

void VectorTextEdit::setVector(const Vector3& value) {
    m_value = value;
    m_valid = true;
    m_error.clear();
    populate();
    emit validityChanged(true, QString());
}

void VectorTextEdit::setFormat(InputFormat format) {
    if (m_format == format) {
        return;
    }

    m_format = format;
    const bool column = format == InputFormat::Column;
    m_ijkPage->setVisible(!column);
    m_columnPage->setVisible(column);
    populate();
    updateFocusProxy();
}

void VectorTextEdit::setReadOnly(bool readOnly) {
    if (m_readOnly == readOnly) {
        return;
    }

    m_readOnly = readOnly;
    m_ijkEdit->setReadOnly(readOnly);
    m_xEdit->setReadOnly(readOnly);
    m_yEdit->setReadOnly(readOnly);
    m_zEdit->setReadOnly(readOnly);
    m_ijkEdit->setFrame(!readOnly);
    m_xEdit->setFrame(!readOnly);
    m_yEdit->setFrame(!readOnly);
    m_zEdit->setFrame(!readOnly);

    if (readOnly) {
        setStyleSheet(QStringLiteral(
            "QLineEdit { background: transparent; border: none; padding: 1px; }"));
    } else {
        setStyleSheet(QString());
    }
}

void VectorTextEdit::populate() {
    const QSignalBlocker blockIjk(m_ijkEdit);
    const QSignalBlocker blockX(m_xEdit);
    const QSignalBlocker blockY(m_yEdit);
    const QSignalBlocker blockZ(m_zEdit);

    m_ijkEdit->setText(VectorExpression::formatIJK(m_value));
    m_xEdit->setText(VectorExpression::formatNumber(m_value.x));
    m_yEdit->setText(VectorExpression::formatNumber(m_value.y));
    m_zEdit->setText(VectorExpression::formatNumber(m_value.z));

    if (m_readOnly) {
        m_ijkEdit->setCursorPosition(0);
        m_xEdit->setCursorPosition(0);
        m_yEdit->setCursorPosition(0);
        m_zEdit->setCursorPosition(0);
    }
}

void VectorTextEdit::handleIjkChanged() {
    if (m_readOnly) {
        return;
    }

    QString error;
    const auto parsed = VectorExpression::parseVector(m_ijkEdit->text(), &error);
    if (parsed.has_value()) {
        acceptValue(*parsed);
    } else {
        rejectValue(error);
    }
}

void VectorTextEdit::handleColumnChanged() {
    if (m_readOnly) {
        return;
    }

    bool okX = false;
    bool okY = false;
    bool okZ = false;
    const double x = m_xEdit->text().trimmed().toDouble(&okX);
    const double y = m_yEdit->text().trimmed().toDouble(&okY);
    const double z = m_zEdit->text().trimmed().toDouble(&okZ);

    if (!okX || !okY || !okZ || !std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) {
        rejectValue(QStringLiteral("Enter a valid number in each row."));
        return;
    }

    acceptValue(Vector3(x, y, z));
}

void VectorTextEdit::acceptValue(const Vector3& value) {
    m_value = value;
    m_valid = true;
    m_error.clear();
    emit valueEdited(value);
    emit validityChanged(true, QString());
}

void VectorTextEdit::rejectValue(const QString& error) {
    m_valid = false;
    m_error = error;
    emit validityChanged(false, error);
}

void VectorTextEdit::updateFocusProxy() {
    if (m_format == InputFormat::Column) {
        setFocusProxy(m_xEdit);
    } else {
        setFocusProxy(m_ijkEdit);
    }
}
