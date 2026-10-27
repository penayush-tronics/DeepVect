#pragma once

#include "GraphTypes.h"
#include "VectorEntry.h"
#include "VectorExpression.h"

#include <QHash>
#include <QList>
#include <QObject>
#include <QString>

struct ThemeColors;

class AppModel : public QObject {
    Q_OBJECT

public:
    explicit AppModel(QObject* parent = nullptr);

    const QList<VectorEntry>& vectors() const;
    int selectedId() const;
    const VectorEntry* vectorById(int id) const;
    bool hasName(const QString& name, int exceptId = -1) const;

    InputFormat inputFormat() const;
    ViewMode viewMode() const;
    Plane2D plane2D() const;
    ThemeMode themeMode() const;
    bool showAxisNumbers() const;
    bool showLabels() const;
    ArrowMode arrowMode() const;
    QString operationExpression() const;

    QHash<QString, Vector3> namedVectors() const;
    OperationResult operationResult() const;
    QList<GraphVector> graphVectors(const ThemeColors& colors) const;

    int addVector(const QString& requestedName = QString(), const Vector3& value = Vector3(1.0, 0.0, 0.0));
    void removeVector(int id);
    bool renameVector(int id, const QString& name, QString* error = nullptr);
    void updateVectorValue(int id, const Vector3& value);
    void setVisible(int id, bool visible);
    void selectVector(int id);

    void setInputFormat(InputFormat format);
    void setViewMode(ViewMode mode);
    void setPlane2D(Plane2D plane);
    void setThemeMode(ThemeMode mode);
    void setShowAxisNumbers(bool show);
    void setShowLabels(bool show);
    void setArrowMode(ArrowMode mode);
    void setOperationExpression(const QString& expression);

    void saveOperationResult();

signals:
    void vectorsChanged();
    void vectorUpdated(int id);
    void selectionChanged(int id);
    void settingsChanged();
    void operationChanged();

private:
    VectorEntry* vectorByIdMutable(int id);
    QString uniqueName(const QString& base) const;
    int nextColorIndex();

    QList<VectorEntry> m_vectors;
    int m_selectedId = -1;
    int m_nextId = 1;
    int m_nextColorIndex = 0;
    InputFormat m_inputFormat = InputFormat::IJK;
    ViewMode m_viewMode = ViewMode::ThreeD;
    Plane2D m_plane2D = Plane2D::XY;
    ThemeMode m_themeMode = ThemeMode::Light;
    bool m_showAxisNumbers = false;
    bool m_showLabels = true;
    ArrowMode m_arrowMode = ArrowMode::Origin;
    QString m_operationExpression = QStringLiteral("A + B");
};
