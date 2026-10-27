#include "OperationCard.h"

#include "AppModel.h"
#include "VectorExpression.h"
#include "VectorTextEdit.h"

#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStyle>
#include <QVBoxLayout>

OperationCard::OperationCard(AppModel* model, QWidget* parent)
    : QFrame(parent)
    , m_model(model) {
    setObjectName(QStringLiteral("Card"));
    setFrameShape(QFrame::NoFrame);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 9, 10, 10);
    layout->setSpacing(6);

    auto* title = new QLabel(QStringLiteral("OPERATION"), this);
    title->setObjectName(QStringLiteral("SectionTitle"));
    layout->addWidget(title);

    m_expressionEdit = new QLineEdit(this);
    m_expressionEdit->setPlaceholderText(QStringLiteral("A + B"));
    m_expressionEdit->setText(m_model->operationExpression());
    layout->addWidget(m_expressionEdit);

    m_hintLabel = new QLabel(
        QStringLiteral("Try A + B, A - B, A . B, A x B, or chained expressions."),
        this);
    m_hintLabel->setObjectName(QStringLiteral("Hint"));
    m_hintLabel->setWordWrap(true);
    layout->addWidget(m_hintLabel);

    m_errorLabel = new QLabel(this);
    m_errorLabel->setObjectName(QStringLiteral("ErrorLabel"));
    m_errorLabel->setWordWrap(true);
    m_errorLabel->hide();
    layout->addWidget(m_errorLabel);

    m_resultContainer = new QWidget(this);
    auto* resultLayout = new QVBoxLayout(m_resultContainer);
    resultLayout->setContentsMargins(0, 6, 0, 0);
    resultLayout->setSpacing(5);

    m_resultTitle = new QLabel(QStringLiteral("RESULT"), m_resultContainer);
    m_resultTitle->setObjectName(QStringLiteral("SectionTitle"));
    resultLayout->addWidget(m_resultTitle);

    m_scalarLabel = new QLabel(m_resultContainer);
    QFont scalarFont = m_scalarLabel->font();
    scalarFont.setPointSize(scalarFont.pointSize() + 5);
    scalarFont.setBold(true);
    m_scalarLabel->setFont(scalarFont);
    m_scalarLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    resultLayout->addWidget(m_scalarLabel);

    m_resultVector = new VectorTextEdit(m_resultContainer);
    m_resultVector->setReadOnly(true);
    resultLayout->addWidget(m_resultVector);

    m_magnitudeLabel = new QLabel(m_resultContainer);
    m_magnitudeLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    resultLayout->addWidget(m_magnitudeLabel);

    m_unitContainer = new QWidget(m_resultContainer);
    auto* unitLayout = new QVBoxLayout(m_unitContainer);
    unitLayout->setContentsMargins(0, 0, 0, 0);
    unitLayout->setSpacing(2);
    auto* unitTitle = new QLabel(QStringLiteral("Unit vector"), m_unitContainer);
    unitTitle->setObjectName(QStringLiteral("Hint"));
    auto* unitRow = new QHBoxLayout;
    unitRow->setContentsMargins(0, 0, 0, 0);
    unitRow->setSpacing(4);
    m_unitLabel = new QLabel(m_unitContainer);
    m_unitLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_unitLabel->setWordWrap(true);
    m_unitUndefined = new QLabel(QStringLiteral("undefined"), m_unitContainer);
    m_unitUndefined->setObjectName(QStringLiteral("Hint"));
    unitRow->addWidget(m_unitLabel, 1);
    unitRow->addWidget(m_unitUndefined);
    unitLayout->addWidget(unitTitle);
    unitLayout->addLayout(unitRow);
    resultLayout->addWidget(m_unitContainer);

    m_saveButton = new QPushButton(QStringLiteral("Save as vector"), m_resultContainer);
    m_saveButton->setObjectName(QStringLiteral("PrimaryButton"));
    resultLayout->addWidget(m_saveButton);

    layout->addWidget(m_resultContainer);
    m_resultContainer->hide();

    connect(m_expressionEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        m_model->setOperationExpression(text);
    });
    connect(m_model, &AppModel::operationChanged, this, &OperationCard::refresh);
    connect(m_model, &AppModel::settingsChanged, this, &OperationCard::refresh);
    connect(m_saveButton, &QPushButton::clicked, this, [this]() {
        m_model->saveOperationResult();
    });

    refresh();
}

void OperationCard::refresh() {
    if (!m_expressionEdit->hasFocus()
        && m_expressionEdit->text() != m_model->operationExpression()) {
        const QSignalBlocker blocker(m_expressionEdit);
        m_expressionEdit->setText(m_model->operationExpression());
    }

    m_resultVector->setFormat(m_model->inputFormat());

    const QString expression = m_model->operationExpression().trimmed();
    if (expression.isEmpty()) {
        clearError();
        m_resultContainer->hide();
        return;
    }

    const OperationResult result = m_model->operationResult();
    if (!result.valid) {
        showError(result.error.isEmpty() ? QStringLiteral("Enter a valid equation.") : result.error);
        m_resultContainer->hide();
        return;
    }

    clearError();
    m_resultContainer->show();

    if (result.value.type == ExpressionValueType::Scalar) {
        m_resultTitle->setText(QStringLiteral("DOT PRODUCT"));
        m_scalarLabel->setText(VectorExpression::formatNumber(result.value.scalar));
        m_scalarLabel->show();
        m_resultVector->hide();
        m_magnitudeLabel->hide();
        m_unitContainer->hide();
        m_saveButton->hide();
        return;
    }

    m_resultTitle->setText(QStringLiteral("RESULT"));
    m_scalarLabel->hide();
    m_resultVector->show();
    m_resultVector->setVector(result.value.vector);

    const double length = result.value.vector.length();
    m_magnitudeLabel->setText(
        QStringLiteral("Magnitude: %1").arg(VectorExpression::formatNumber(length)));
    m_magnitudeLabel->show();
    m_unitContainer->show();
    m_saveButton->show();

    const bool isZero = length < 1e-12;
    m_unitLabel->setVisible(!isZero);
    m_unitUndefined->setVisible(isZero);
    if (!isZero) {
        m_unitLabel->setText(VectorExpression::formatUnitFraction(
            result.value.vector, m_model->inputFormat() == InputFormat::Column));
    }
}

void OperationCard::clearError() {
    m_errorLabel->clear();
    m_errorLabel->hide();
    setProperty("error", false);
    style()->unpolish(this);
    style()->polish(this);
}

void OperationCard::showError(const QString& error) {
    m_errorLabel->setText(error);
    m_errorLabel->show();
    setProperty("error", true);
    style()->unpolish(this);
    style()->polish(this);
}
