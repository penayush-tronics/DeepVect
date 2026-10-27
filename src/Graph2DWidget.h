#pragma once

#include "GraphTypes.h"
#include "Theme.h"

#include <QList>
#include <QPointF>
#include <QWidget>

class QMouseEvent;
class QPainter;
class QResizeEvent;
class QWheelEvent;

class Graph2DWidget : public QWidget {
    Q_OBJECT

public:
    explicit Graph2DWidget(QWidget* parent = nullptr);

    void setVectors(const QList<GraphVector>& vectors);
    void setPlane(Plane2D plane);
    void setShowAxisNumbers(bool show);
    void setShowLabels(bool show);
    void setTheme(const ThemeColors& colors);
    void fitToScene();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    QPointF toScreen(const QPointF& planePoint) const;
    QPointF toPlane(const QPointF& screenPoint) const;
    QPointF project(const Vector3& value) const;

    void drawGrid(QPainter& painter) const;
    void drawAxes(QPainter& painter) const;
    void drawVectors(QPainter& painter) const;
    void drawTickLabels(QPainter& painter, double step) const;

    double niceStep(double target) const;
    QString formatTick(double value, double step) const;
    void axisLabels(QString* horizontal, QString* vertical, QColor* horizontalColor,
                    QColor* verticalColor) const;

    QList<GraphVector> m_vectors;
    Plane2D m_plane = Plane2D::XY;
    bool m_showAxisNumbers = false;
    bool m_showLabels = true;
    ThemeColors m_colors = Theme::colors(ThemeMode::Light);

    QPointF m_center;
    double m_scale = 60.0;
    bool m_needsFit = true;
    bool m_panning = false;
    QPointF m_lastMousePos;
};
