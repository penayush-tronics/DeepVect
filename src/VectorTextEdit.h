#pragma once

#include "GraphTypes.h"
#include "Vector3.h"

#include <QWidget>

class QLabel;
class QLineEdit;
class QVBoxLayout;

class VectorTextEdit : public QWidget {
    Q_OBJECT

public:
    explicit VectorTextEdit(QWidget* parent = nullptr);

    Vector3 value() const;
    InputFormat format() const;
    bool isValid() const;
    QString errorText() const;
    bool isReadOnly() const;

    void setVector(const Vector3& value);
    void setFormat(InputFormat format);
    void setReadOnly(bool readOnly);

signals:
    void valueEdited(const Vector3& value);
    void validityChanged(bool valid, const QString& error);

private:
    void populate();
    void handleIjkChanged();
    void handleColumnChanged();
    void acceptValue(const Vector3& value);
    void rejectValue(const QString& error);
    void updateFocusProxy();

    QWidget* m_ijkPage = nullptr;
    QWidget* m_columnPage = nullptr;
    QVBoxLayout* m_stack = nullptr;
    QLineEdit* m_ijkEdit = nullptr;
    QLineEdit* m_xEdit = nullptr;
    QLineEdit* m_yEdit = nullptr;
    QLineEdit* m_zEdit = nullptr;

    InputFormat m_format = InputFormat::IJK;
    Vector3 m_value;
    bool m_valid = true;
    bool m_readOnly = false;
    QString m_error;
};
