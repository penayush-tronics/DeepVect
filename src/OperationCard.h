#pragma once

#include <QFrame>
#include <QString>

class AppModel;
class QLabel;
class QLineEdit;
class QPushButton;
class QWidget;
class VectorTextEdit;

class OperationCard : public QFrame {
    Q_OBJECT

public:
    explicit OperationCard(AppModel* model, QWidget* parent = nullptr);

    void refresh();

private:
    void clearError();
    void showError(const QString& error);

    AppModel* m_model = nullptr;
    QLineEdit* m_expressionEdit = nullptr;
    QLabel* m_hintLabel = nullptr;
    QLabel* m_errorLabel = nullptr;
    QWidget* m_resultContainer = nullptr;
    QLabel* m_resultTitle = nullptr;
    QLabel* m_scalarLabel = nullptr;
    VectorTextEdit* m_resultVector = nullptr;
    QLabel* m_magnitudeLabel = nullptr;
    QWidget* m_unitContainer = nullptr;
    QLabel* m_unitLabel = nullptr;
    QLabel* m_unitUndefined = nullptr;
    QPushButton* m_saveButton = nullptr;
};
