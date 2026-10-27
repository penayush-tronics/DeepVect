#pragma once

#include "GraphTypes.h"
#include "Theme.h"
#include "Vector3.h"

#include <QList>
#include <QMatrix4x4>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLWidget>
#include <QPointF>
#include <QVector3D>

#include <optional>

class QMouseEvent;
class QOpenGLShaderProgram;
class QPainter;
class QWheelEvent;

class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core {
    Q_OBJECT

public:
    explicit GLWidget(QWidget* parent = nullptr);
    ~GLWidget() override;

    void setGraphVectors(const QList<GraphVector>& vectors);
    void setShowAxisNumbers(bool show);
    void setShowLabels(bool show);
    void setTheme(const ThemeColors& colors);
    void fitToScene();
    void resetCamera();

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    void drawLine(const Vector3& from, const Vector3& to, const QVector3D& color,
                  float width = 1.0f);
    void drawDashedLine(const Vector3& from, const Vector3& to, const QVector3D& color,
                        int dashes, float width = 1.0f);
    void drawArrow(const GraphVector& vector, double gridStepValue);
    void drawAxes(double extent);
    void drawGrid(double step, int halfLines);
    void computeGrid(double* step, int* halfLines) const;

    QVector3D eyePosition() const;
    QMatrix4x4 viewMatrix() const;
    QMatrix4x4 projectionMatrix() const;

    void drawOverlay(QPainter& painter, const QMatrix4x4& mvp, double gridStepValue,
                     double extent);
    std::optional<QPointF> projectToScreen(const QMatrix4x4& mvp, const Vector3& world) const;

    QList<GraphVector> m_vectors;
    ThemeColors m_colors = Theme::colors(ThemeMode::Light);
    bool m_showAxisNumbers = false;
    bool m_showLabels = true;

    QOpenGLShaderProgram* m_program = nullptr;
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    bool m_glReady = false;
    bool m_needsFit = true;

    float m_yaw = 45.0f;
    float m_pitch = 25.0f;
    float m_distance = 15.0f;
    QPointF m_lastMousePos;
    bool m_dragging = false;
};
