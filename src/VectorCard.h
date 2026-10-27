#pragma once

#include "GraphTypes.h"
#include "Vector3.h"

#include <QFrame>

class AppModel;
class QCheckBox;
class QLabel;
class QLineEdit;
class QToolButton;
class VectorTextEdit;
struct VectorEntry;

class VectorCard : public QFrame {
    Q_OBJECT

public:
    VectorCard(AppModel* model, int vectorId, QWidget* parent = nullptr);

    int vectorId() const;
    void refresh();
    void setSelected(bool selected);
    void setInputFormat(InputFormat format);

signals:
    void activated(int id);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void installActivationFilter(QWidget* widget);
    void updateStats(const VectorEntry& entry);
    void clearError();
    void showError(const QString& error);

    AppModel* m_model = nullptr;
    int m_id = -1;

    QLabel* m_colorDot = nullptr;
    QLineEdit* m_nameEdit = nullptr;
    QCheckBox* m_visibleButton = nullptr;
    QToolButton* m_deleteButton = nullptr;
    VectorTextEdit* m_vectorEdit = nullptr;
    QLabel* m_unitLabel = nullptr;
    QLabel* m_unitUndefined = nullptr;
    QLabel* m_magnitudeLabel = nullptr;
    QLabel* m_directionLabel = nullptr;
    QLabel* m_errorLabel = nullptr;
    QWidget* m_statsWidget = nullptr;
};
