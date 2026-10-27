#include "GLWidget.h"

#include <QColor>
#include <QFont>
#include <QFontMetricsF>
#include <QMouseEvent>
#include <QOpenGLShaderProgram>
#include <QPainter>
#include <QVector4D>
#include <QWheelEvent>
#include <QtMath>

#include <algorithm>
#include <cmath>

namespace {

QVector3D toVec(const QColor& color) {
    return QVector3D(
        static_cast<float>(color.redF()),
        static_cast<float>(color.greenF()),
        static_cast<float>(color.blueF()));
}

double niceStep(double target) {
    if (!(target > 0.0) || !std::isfinite(target)) {
        return 1.0;
    }
    const double exponent = std::floor(std::log10(target));
    const double base = std::pow(10.0, exponent);
    const double fraction = target / base;

    double nice = 10.0;
    if (fraction <= 1.0) {
        nice = 1.0;
    } else if (fraction <= 2.0) {
        nice = 2.0;
    } else if (fraction <= 5.0) {
        nice = 5.0;
    }
    return nice * base;
}

QString formatTick(double value, double step) {
    if (std::abs(value) < step * 1e-6) {
        return QStringLiteral("0");
    }

    int decimals = 0;
    if (step < 1.0) {
        decimals = static_cast<int>(std::ceil(-std::log10(step)));
        decimals = std::clamp(decimals, 0, 6);
    }

    QString text = QString::number(value, 'f', decimals);
    if (text.contains(QLatin1Char('.'))) {
        while (text.endsWith(QLatin1Char('0'))) {
            text.chop(1);
        }
        if (text.endsWith(QLatin1Char('.'))) {
            text.chop(1);
        }
    }
    return text;
}

} // namespace

GLWidget::GLWidget(QWidget* parent)
    : QOpenGLWidget(parent) {
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(400, 400);
    setCursor(Qt::OpenHandCursor);
}

GLWidget::~GLWidget() {
    makeCurrent();

    if (m_program != nullptr) {
        delete m_program;
        m_program = nullptr;
    }
    if (m_vbo != 0) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }

    doneCurrent();
}

void GLWidget::setGraphVectors(const QList<GraphVector>& vectors) {
    m_vectors = vectors;
    update();
}

void GLWidget::setShowAxisNumbers(bool show) {
    if (m_showAxisNumbers == show) {
        return;
    }
    m_showAxisNumbers = show;
    update();
}

void GLWidget::setShowLabels(bool show) {
    if (m_showLabels == show) {
        return;
    }
    m_showLabels = show;
    update();
}

void GLWidget::setTheme(const ThemeColors& colors) {
    m_colors = colors;
    update();
}

void GLWidget::fitToScene() {
    double maxExtent = 1.0;
    for (const GraphVector& vector : m_vectors) {
        const Vector3 tip = vector.tail + vector.value;
        maxExtent = std::max({maxExtent,
                              std::abs(vector.tail.x),
                              std::abs(vector.tail.y),
                              std::abs(vector.tail.z),
                              std::abs(tip.x),
                              std::abs(tip.y),
                              std::abs(tip.z)});
    }

    const double distance = std::clamp(maxExtent * 3.2, 6.0, 500.0);
    m_distance = static_cast<float>(distance);
    m_needsFit = false;
    update();
}

void GLWidget::resetCamera() {
    m_yaw = 45.0f;
    m_pitch = 25.0f;
    m_needsFit = true;
    update();
}

void GLWidget::initializeGL() {
    initializeOpenGLFunctions();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    const char* vertexShaderSource = R"(#version 330 core
layout(location = 0) in vec3 aPos;
uniform mat4 uMvp;
void main() {
    gl_Position = uMvp * vec4(aPos, 1.0);
}
)";

    const char* fragmentShaderSource = R"(#version 330 core
out vec4 FragColor;
uniform vec3 uColor;
void main() {
    FragColor = vec4(uColor, 1.0);
}
)";

    m_program = new QOpenGLShaderProgram;
    if (!m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource)) {
        qWarning() << "Vertex shader compilation failed:" << m_program->log();
        delete m_program;
        m_program = nullptr;
        return;
    }
    if (!m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource)) {
        qWarning() << "Fragment shader compilation failed:" << m_program->log();
        delete m_program;
        m_program = nullptr;
        return;
    }
    if (!m_program->link()) {
        qWarning() << "Shader link failed:" << m_program->log();
        delete m_program;
        m_program = nullptr;
        return;
    }

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    m_glReady = true;
}

void GLWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}

void GLWidget::paintGL() {
    QPainter painter(this);
    painter.beginNativePainting();

    glClearColor(
        static_cast<GLfloat>(m_colors.canvas.redF()),
        static_cast<GLfloat>(m_colors.canvas.greenF()),
        static_cast<GLfloat>(m_colors.canvas.blueF()),
        1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const int w = width();
    const int h = height();
    QMatrix4x4 mvp;
    double gridStepValue = 1.0;
    int halfLines = 10;
    bool drewScene = false;

    if (m_glReady && m_program != nullptr && w > 0 && h > 0) {
        if (m_needsFit) {
            fitToScene();
        }

        mvp = projectionMatrix() * viewMatrix();
        computeGrid(&gridStepValue, &halfLines);

        m_program->bind();
        m_program->setUniformValue("uMvp", mvp);
        glBindVertexArray(m_vao);

        drawGrid(gridStepValue, halfLines);
        drawAxes(static_cast<double>(halfLines) * gridStepValue);

        for (const GraphVector& vector : m_vectors) {
            if (!vector.selected && !vector.isResult) {
                drawArrow(vector, gridStepValue);
            }
        }
        for (const GraphVector& vector : m_vectors) {
            if (vector.isResult) {
                drawArrow(vector, gridStepValue);
            }
        }
        for (const GraphVector& vector : m_vectors) {
            if (vector.selected) {
                drawArrow(vector, gridStepValue);
            }
        }

        glBindVertexArray(0);
        m_program->release();
        drewScene = true;
    }

    painter.endNativePainting();

    if (drewScene) {
        painter.setRenderHint(QPainter::Antialiasing, true);
        drawOverlay(painter, mvp, gridStepValue,
                    static_cast<double>(halfLines) * gridStepValue);
    }
}

void GLWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_lastMousePos = event->position();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    QOpenGLWidget::mousePressEvent(event);
}

void GLWidget::mouseMoveEvent(QMouseEvent* event) {
    if (m_dragging) {
        const QPointF currentPos = event->position();
        const QPointF delta = currentPos - m_lastMousePos;
        m_lastMousePos = currentPos;

        m_yaw += static_cast<float>(delta.x()) * 0.5f;
        m_pitch -= static_cast<float>(delta.y()) * 0.5f;
        m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);

        update();
        event->accept();
        return;
    }
    QOpenGLWidget::mouseMoveEvent(event);
}

void GLWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        setCursor(Qt::OpenHandCursor);
        event->accept();
        return;
    }
    QOpenGLWidget::mouseReleaseEvent(event);
}

void GLWidget::wheelEvent(QWheelEvent* event) {
    const int delta = event->angleDelta().y();
    if (delta != 0) {
        m_distance *= (delta > 0) ? 0.9f : 1.1f;
        m_distance = std::clamp(m_distance, 2.0f, 1000.0f);
        update();
        event->accept();
        return;
    }
    QOpenGLWidget::wheelEvent(event);
}

QVector3D GLWidget::eyePosition() const {
    const float yawRad = static_cast<float>(qDegreesToRadians(static_cast<double>(m_yaw)));
    const float pitchRad = static_cast<float>(qDegreesToRadians(static_cast<double>(m_pitch)));
    const float cosPitch = std::cos(pitchRad);

    return QVector3D(
        m_distance * cosPitch * std::sin(yawRad),
        m_distance * std::sin(pitchRad),
        m_distance * cosPitch * std::cos(yawRad));
}

QMatrix4x4 GLWidget::viewMatrix() const {
    QMatrix4x4 view;
    view.lookAt(eyePosition(), QVector3D(0.0f, 0.0f, 0.0f), QVector3D(0.0f, 1.0f, 0.0f));
    return view;
}

QMatrix4x4 GLWidget::projectionMatrix() const {
    const float aspect = static_cast<float>(width()) / static_cast<float>(std::max(1, height()));
    QMatrix4x4 projection;
    projection.perspective(45.0f, aspect, 0.1f, 2000.0f);
    return projection;
}

void GLWidget::computeGrid(double* step, int* halfLines) const {
    double maxExtent = 5.0;
    for (const GraphVector& vector : m_vectors) {
        const Vector3 tip = vector.tail + vector.value;
        maxExtent = std::max({maxExtent,
                              std::abs(vector.tail.x),
                              std::abs(vector.tail.y),
                              std::abs(vector.tail.z),
                              std::abs(tip.x),
                              std::abs(tip.y),
                              std::abs(tip.z)});
    }

    const double base =
        std::max({5.0, maxExtent * 1.4, static_cast<double>(m_distance) * 0.7});
    *step = niceStep(base / 10.0);
    *halfLines = std::clamp(static_cast<int>(std::ceil(base / *step)), 5, 200);
}

void GLWidget::drawLine(const Vector3& from, const Vector3& to, const QVector3D& color,
                        float width) {
    const float vertices[6] = {
        static_cast<float>(from.x),
        static_cast<float>(from.y),
        static_cast<float>(from.z),
        static_cast<float>(to.x),
        static_cast<float>(to.y),
        static_cast<float>(to.z),
    };

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    m_program->setUniformValue("uColor", color);
    glLineWidth(std::max(1.0f, width));
    glDrawArrays(GL_LINES, 0, 2);
}

void GLWidget::drawDashedLine(const Vector3& from, const Vector3& to, const QVector3D& color,
                              int dashes, float width) {
    const Vector3 delta = to - from;
    if (delta.lengthSquared() < 1e-12) {
        return;
    }

    const int count = std::max(1, dashes);
    const int segments = count * 2 - 1;
    for (int i = 0; i < segments; i += 2) {
        const double t0 = static_cast<double>(i) / static_cast<double>(segments);
        const double t1 = static_cast<double>(i + 1) / static_cast<double>(segments);
        drawLine(from + delta * t0, from + delta * t1, color, width);
    }
}

void GLWidget::drawArrow(const GraphVector& vector, double gridStepValue) {
    const Vector3& from = vector.tail;
    const Vector3& vec = vector.value;
    const Vector3 tip = from + vec;
    const double length = vec.length();
    if (length < 1e-9) {
        return;
    }

    const QVector3D color = toVec(vector.color.isValid() ? vector.color : m_colors.accent);
    const float lineWidth = vector.selected ? 4.0f : (vector.isResult ? 3.1f : 2.9f);
    if (vector.isResult) {
        drawDashedLine(from, tip, color, 12, lineWidth);
    } else {
        drawLine(from, tip, color, lineWidth);
    }

    const Vector3 direction = vec / length;

    Vector3 reference(0.0, 1.0, 0.0);
    if (std::abs(direction.dot(reference)) > 0.99) {
        reference = Vector3(1.0, 0.0, 0.0);
    }

    Vector3 side = direction.cross(reference).normalized();
    if (side.lengthSquared() < 1e-12) {
        reference = Vector3(0.0, 0.0, 1.0);
        side = direction.cross(reference).normalized();
    }
    const Vector3 up = side.cross(direction).normalized();

    double headLength = std::clamp(length * 0.18, gridStepValue * 0.25, gridStepValue * 0.9);
    headLength = std::min(headLength, length * 0.35);
    const double headWidth = headLength * 0.5;
    const Vector3 base = tip - direction * headLength;

    drawLine(tip, base + side * headWidth, color, lineWidth);
    drawLine(tip, base - side * headWidth, color, lineWidth);
    drawLine(tip, base + up * headWidth, color, lineWidth);
    drawLine(tip, base - up * headWidth, color, lineWidth);
}

void GLWidget::drawGrid(double step, int halfLines) {
    const double limit = static_cast<double>(halfLines) * step;
    const QVector3D minorColor = toVec(m_colors.gridMinor);
    const QVector3D majorColor = toVec(m_colors.gridMajor);

    for (int i = -halfLines; i <= halfLines; ++i) {
        if (i == 0) {
            continue;
        }
        const double coord = static_cast<double>(i) * step;
        drawLine(Vector3(coord, 0.0, -limit), Vector3(coord, 0.0, limit), minorColor);
        drawLine(Vector3(-limit, 0.0, coord), Vector3(limit, 0.0, coord), minorColor);
    }

    for (int i = -halfLines; i <= halfLines; ++i) {
        if (i == 0 || i % 5 != 0) {
            continue;
        }
        const double coord = static_cast<double>(i) * step;
        drawLine(Vector3(coord, 0.0, -limit), Vector3(coord, 0.0, limit), majorColor);
        drawLine(Vector3(-limit, 0.0, coord), Vector3(limit, 0.0, coord), majorColor);
    }
}

void GLWidget::drawAxes(double extent) {
    drawLine(
        Vector3(-extent, 0.0, 0.0),
        Vector3(extent, 0.0, 0.0),
        toVec(m_colors.axisX), 1.8f);
    drawLine(
        Vector3(0.0, -extent, 0.0),
        Vector3(0.0, extent, 0.0),
        toVec(m_colors.axisY), 1.8f);
    drawLine(
        Vector3(0.0, 0.0, -extent),
        Vector3(0.0, 0.0, extent),
        toVec(m_colors.axisZ), 1.8f);
}

std::optional<QPointF> GLWidget::projectToScreen(const QMatrix4x4& mvp,
                                                 const Vector3& world) const {
    const QVector4D clip = mvp * QVector4D(
                                     static_cast<float>(world.x),
                                     static_cast<float>(world.y),
                                     static_cast<float>(world.z),
                                     1.0f);
    if (clip.w() <= 1e-6f) {
        return std::nullopt;
    }

    const double ndcX = clip.x() / clip.w();
    const double ndcY = clip.y() / clip.w();
    if (ndcX < -1.25 || ndcX > 1.25 || ndcY < -1.25 || ndcY > 1.25) {
        return std::nullopt;
    }

    return QPointF(
        (ndcX * 0.5 + 0.5) * width(),
        (1.0 - (ndcY * 0.5 + 0.5)) * height());
}

void GLWidget::drawOverlay(QPainter& painter, const QMatrix4x4& mvp, double gridStepValue,
                           double extent) {
    painter.save();

    if (m_showAxisNumbers) {
        const double labelStep = gridStepValue * 2.0;
        QFont tickFont = painter.font();
        tickFont.setPointSizeF(8.5);
        painter.setFont(tickFont);
        painter.setPen(m_colors.mutedText);

        const int count = static_cast<int>(std::floor(extent / labelStep + 1e-9));
        for (int i = -count; i <= count; ++i) {
            if (i == 0) {
                continue;
            }
            const double value = static_cast<double>(i) * labelStep;
            const QString text = formatTick(value, labelStep);

            if (const auto screen = projectToScreen(mvp, Vector3(value, 0.0, 0.0))) {
                painter.drawText(*screen + QPointF(5.0, 12.0), text);
            }
            if (const auto screen = projectToScreen(mvp, Vector3(0.0, value, 0.0))) {
                painter.drawText(*screen + QPointF(7.0, 4.0), text);
            }
            if (const auto screen = projectToScreen(mvp, Vector3(0.0, 0.0, value))) {
                painter.drawText(*screen + QPointF(5.0, 12.0), text);
            }
        }

        if (const auto origin = projectToScreen(mvp, Vector3())) {
            painter.drawText(*origin + QPointF(5.0, 12.0), QStringLiteral("0"));
        }
    }

    QFont axisFont = painter.font();
    axisFont.setPointSizeF(12.5);
    axisFont.setBold(true);
    painter.setFont(axisFont);

    const double letterPosition = extent + gridStepValue * 0.6;
    if (const auto screen = projectToScreen(mvp, Vector3(letterPosition, 0.0, 0.0))) {
        painter.setPen(m_colors.axisX);
        painter.drawText(*screen, QStringLiteral("x"));
    }
    if (const auto screen = projectToScreen(mvp, Vector3(0.0, letterPosition, 0.0))) {
        painter.setPen(m_colors.axisY);
        painter.drawText(*screen, QStringLiteral("y"));
    }
    if (const auto screen = projectToScreen(mvp, Vector3(0.0, 0.0, letterPosition))) {
        painter.setPen(m_colors.axisZ);
        painter.drawText(*screen, QStringLiteral("z"));
    }

    for (const GraphVector& vector : m_vectors) {
        const auto screen = projectToScreen(mvp, vector.tail + vector.value);
        if (!screen) {
            continue;
        }

        if (vector.selected) {
            const QColor color = vector.color.isValid() ? vector.color : m_colors.accent;
            painter.setPen(QPen(m_colors.panel, 1.5));
            painter.setBrush(color);
            painter.drawEllipse(*screen, 4.0, 4.0);
        }

        if (!m_showLabels || vector.name.isEmpty()) {
            continue;
        }

        QFont font = painter.font();
        font.setPointSizeF(10.0);
        font.setBold(vector.selected);
        painter.setFont(font);
        const QFontMetricsF metrics(font);
        const QString label = metrics.elidedText(vector.name, Qt::ElideRight, 190.0);

        const double textWidth = metrics.horizontalAdvance(label);
        const double textHeight = metrics.height();
        QPointF topLeft(screen->x() + 10.0, screen->y() - textHeight - 8.0);
        if (topLeft.x() + textWidth + 10.0 > width() - 4.0) {
            topLeft.setX(screen->x() - textWidth - 14.0);
        }
        if (topLeft.y() < 4.0) {
            topLeft.setY(screen->y() + 10.0);
        }
        if (topLeft.y() + textHeight + 8.0 > height() - 4.0) {
            topLeft.setY(height() - textHeight - 8.0);
        }
        if (topLeft.x() < 4.0) {
            topLeft.setX(4.0);
        }

        const QRectF box(
            topLeft.x() - 4.0, topLeft.y() - 2.0, textWidth + 8.0, textHeight + 4.0);
        painter.setPen(QPen(m_colors.border, 1.0));
        painter.setBrush(m_colors.panel);
        painter.drawRoundedRect(box, 4.0, 4.0);

        painter.setPen(vector.isResult ? m_colors.result : m_colors.text);
        painter.drawText(QPointF(topLeft.x(), topLeft.y() + metrics.ascent()), label);
    }

    painter.restore();
}
