#include "Graph2DWidget.h"

#include <QFont>
#include <QFontMetrics>
#include <QFontMetricsF>
#include <QLineF>
#include <QMouseEvent>
#include <QPainter>
#include <QPolygonF>
#include <QResizeEvent>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>

namespace {

void drawArrowHead(QPainter& painter, const QPointF& tip, const QPointF& direction,
                   const QPointF& perpendicular, double size) {
    const QPointF back = tip - direction * size;
    painter.drawLine(tip, back + perpendicular * (size * 0.45));
    painter.drawLine(tip, back - perpendicular * (size * 0.45));
}

} // namespace

Graph2DWidget::Graph2DWidget(QWidget* parent)
    : QWidget(parent) {
    setMinimumSize(360, 360);
    setFocusPolicy(Qt::StrongFocus);
    setCursor(Qt::OpenHandCursor);
}

void Graph2DWidget::setVectors(const QList<GraphVector>& vectors) {
    m_vectors = vectors;
    update();
}

void Graph2DWidget::setPlane(Plane2D plane) {
    if (m_plane == plane) {
        return;
    }
    m_plane = plane;
    m_needsFit = true;
    if (isVisible()) {
        fitToScene();
    } else {
        update();
    }
}

void Graph2DWidget::setShowAxisNumbers(bool show) {
    if (m_showAxisNumbers == show) {
        return;
    }
    m_showAxisNumbers = show;
    update();
}

void Graph2DWidget::setShowLabels(bool show) {
    if (m_showLabels == show) {
        return;
    }
    m_showLabels = show;
    update();
}

void Graph2DWidget::setTheme(const ThemeColors& colors) {
    m_colors = colors;
    update();
}

void Graph2DWidget::fitToScene() {
    if (width() <= 0 || height() <= 0) {
        m_needsFit = true;
        return;
    }

    if (m_vectors.isEmpty()) {
        m_center = QPointF(0.0, 0.0);
        m_scale = 60.0;
        m_needsFit = false;
        update();
        return;
    }

    double minX = 0.0;
    double maxX = 0.0;
    double minY = 0.0;
    double maxY = 0.0;
    bool first = true;
    auto include = [&](double x, double y) {
        if (first) {
            minX = maxX = x;
            minY = maxY = y;
            first = false;
            return;
        }
        minX = std::min(minX, x);
        maxX = std::max(maxX, x);
        minY = std::min(minY, y);
        maxY = std::max(maxY, y);
    };

    include(0.0, 0.0);
    for (const GraphVector& vector : m_vectors) {
        const QPointF start = project(vector.tail);
        const QPointF end = project(vector.tail + vector.value);
        include(start.x(), start.y());
        include(end.x(), end.y());
    }

    double rangeX = maxX - minX;
    double rangeY = maxY - minY;
    if (rangeX < 1e-9 && rangeY < 1e-9) {
        rangeX = 10.0;
        rangeY = 10.0;
    } else if (rangeX < 1e-9) {
        rangeX = rangeY * 0.5;
    } else if (rangeY < 1e-9) {
        rangeY = rangeX * 0.5;
    }

    const double paddedX = rangeX * 1.25;
    const double paddedY = rangeY * 1.25;
    const double usableWidth = std::max(120.0, static_cast<double>(width()) - 80.0);
    const double usableHeight = std::max(120.0, static_cast<double>(height()) - 80.0);

    m_scale = std::min(usableWidth / paddedX, usableHeight / paddedY);
    m_scale = std::clamp(m_scale, 1.0, 5000.0);
    m_center = QPointF((minX + maxX) * 0.5, (minY + maxY) * 0.5);
    m_needsFit = false;
    update();
}

void Graph2DWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), m_colors.canvas);

    drawGrid(painter);
    drawAxes(painter);
    drawVectors(painter);
}

void Graph2DWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    if (m_needsFit) {
        fitToScene();
    }
}

void Graph2DWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_panning = true;
        m_lastMousePos = event->position();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void Graph2DWidget::mouseMoveEvent(QMouseEvent* event) {
    if (m_panning) {
        const QPointF delta = event->position() - m_lastMousePos;
        m_lastMousePos = event->position();
        m_center -= QPointF(delta.x() / m_scale, -delta.y() / m_scale);
        update();
        event->accept();
        return;
    }
    QWidget::mouseMoveEvent(event);
}

void Graph2DWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && m_panning) {
        m_panning = false;
        setCursor(Qt::OpenHandCursor);
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void Graph2DWidget::wheelEvent(QWheelEvent* event) {
    const int delta = event->angleDelta().y();
    if (delta == 0) {
        QWidget::wheelEvent(event);
        return;
    }

    const QPointF screenPos = event->position();
    const QPointF before = toPlane(screenPos);
    const double factor = delta > 0 ? 1.15 : 1.0 / 1.15;
    m_scale = std::clamp(m_scale * factor, 1.0, 5000.0);
    const QPointF after = toPlane(screenPos);
    m_center += before - after;

    update();
    event->accept();
}

QPointF Graph2DWidget::toScreen(const QPointF& planePoint) const {
    return QPointF(
        width() * 0.5 + (planePoint.x() - m_center.x()) * m_scale,
        height() * 0.5 - (planePoint.y() - m_center.y()) * m_scale);
}

QPointF Graph2DWidget::toPlane(const QPointF& screenPoint) const {
    return QPointF(
        m_center.x() + (screenPoint.x() - width() * 0.5) / m_scale,
        m_center.y() - (screenPoint.y() - height() * 0.5) / m_scale);
}

QPointF Graph2DWidget::project(const Vector3& value) const {
    switch (m_plane) {
    case Plane2D::XY:
        return QPointF(value.x, value.y);
    case Plane2D::XZ:
        return QPointF(value.x, value.z);
    case Plane2D::YZ:
        return QPointF(value.y, value.z);
    }
    return QPointF(0.0, 0.0);
}

double Graph2DWidget::niceStep(double target) const {
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

QString Graph2DWidget::formatTick(double value, double step) const {
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

void Graph2DWidget::axisLabels(QString* horizontal, QString* vertical, QColor* horizontalColor,
                               QColor* verticalColor) const {
    switch (m_plane) {
    case Plane2D::XY:
        *horizontal = QStringLiteral("x");
        *vertical = QStringLiteral("y");
        *horizontalColor = m_colors.axisX;
        *verticalColor = m_colors.axisY;
        break;
    case Plane2D::XZ:
        *horizontal = QStringLiteral("x");
        *vertical = QStringLiteral("z");
        *horizontalColor = m_colors.axisX;
        *verticalColor = m_colors.axisZ;
        break;
    case Plane2D::YZ:
        *horizontal = QStringLiteral("y");
        *vertical = QStringLiteral("z");
        *horizontalColor = m_colors.axisY;
        *verticalColor = m_colors.axisZ;
        break;
    }
}

void Graph2DWidget::drawGrid(QPainter& painter) const {
    const QPointF topLeft = toPlane(QPointF(0.0, 0.0));
    const QPointF bottomRight = toPlane(QPointF(width(), height()));
    const double xMin = std::min(topLeft.x(), bottomRight.x());
    const double xMax = std::max(topLeft.x(), bottomRight.x());
    const double yMin = std::min(topLeft.y(), bottomRight.y());
    const double yMax = std::max(topLeft.y(), bottomRight.y());

    const double step = niceStep(80.0 / m_scale);
    const double minor = step / 5.0;

    const long long xMinorStart = static_cast<long long>(std::ceil(xMin / minor));
    const long long xMinorEnd = static_cast<long long>(std::floor(xMax / minor));
    const long long yMinorStart = static_cast<long long>(std::ceil(yMin / minor));
    const long long yMinorEnd = static_cast<long long>(std::floor(yMax / minor));

    painter.save();
    painter.setPen(QPen(m_colors.gridMinor, 1.0));
    for (long long i = xMinorStart; i <= xMinorEnd; ++i) {
        const double x = static_cast<double>(i) * minor;
        const double screenX = toScreen(QPointF(x, 0.0)).x();
        painter.drawLine(QPointF(screenX, 0.0), QPointF(screenX, height()));
    }
    for (long long i = yMinorStart; i <= yMinorEnd; ++i) {
        const double y = static_cast<double>(i) * minor;
        const double screenY = toScreen(QPointF(0.0, y)).y();
        painter.drawLine(QPointF(0.0, screenY), QPointF(width(), screenY));
    }

    painter.setPen(QPen(m_colors.gridMajor, 1.0));
    const long long xMajorStart = static_cast<long long>(std::ceil(xMin / step));
    const long long xMajorEnd = static_cast<long long>(std::floor(xMax / step));
    const long long yMajorStart = static_cast<long long>(std::ceil(yMin / step));
    const long long yMajorEnd = static_cast<long long>(std::floor(yMax / step));
    for (long long i = xMajorStart; i <= xMajorEnd; ++i) {
        const double x = static_cast<double>(i) * step;
        const double screenX = toScreen(QPointF(x, 0.0)).x();
        painter.drawLine(QPointF(screenX, 0.0), QPointF(screenX, height()));
    }
    for (long long i = yMajorStart; i <= yMajorEnd; ++i) {
        const double y = static_cast<double>(i) * step;
        const double screenY = toScreen(QPointF(0.0, y)).y();
        painter.drawLine(QPointF(0.0, screenY), QPointF(width(), screenY));
    }
    painter.restore();
}

void Graph2DWidget::drawAxes(QPainter& painter) const {
    const QPointF origin = toScreen(QPointF(0.0, 0.0));
    const bool horizontalVisible = origin.y() >= 0.0 && origin.y() <= height();
    const bool verticalVisible = origin.x() >= 0.0 && origin.x() <= width();

    painter.save();
    painter.setPen(QPen(m_colors.axis, 1.3));
    if (horizontalVisible) {
        painter.drawLine(QPointF(0.0, origin.y()), QPointF(width(), origin.y()));
        QPolygonF arrow;
        arrow << QPointF(width() - 6.0, origin.y())
              << QPointF(width() - 15.0, origin.y() - 4.0)
              << QPointF(width() - 15.0, origin.y() + 4.0);
        painter.setBrush(m_colors.axis);
        painter.drawPolygon(arrow);
        painter.setBrush(Qt::NoBrush);
    }
    if (verticalVisible) {
        painter.drawLine(QPointF(origin.x(), 0.0), QPointF(origin.x(), height()));
        QPolygonF arrow;
        arrow << QPointF(origin.x(), 6.0)
              << QPointF(origin.x() - 4.0, 15.0)
              << QPointF(origin.x() + 4.0, 15.0);
        painter.setBrush(m_colors.axis);
        painter.drawPolygon(arrow);
        painter.setBrush(Qt::NoBrush);
    }

    QString horizontalLabel;
    QString verticalLabel;
    QColor horizontalColor;
    QColor verticalColor;
    axisLabels(&horizontalLabel, &verticalLabel, &horizontalColor, &verticalColor);

    QFont labelFont = painter.font();
    labelFont.setPointSizeF(11.0);
    labelFont.setBold(true);
    painter.setFont(labelFont);

    if (horizontalVisible) {
        painter.setPen(horizontalColor);
        painter.drawText(
            QRectF(width() - 28.0, origin.y() - 22.0, 18.0, 18.0),
            Qt::AlignCenter,
            horizontalLabel);
    }
    if (verticalVisible) {
        painter.setPen(verticalColor);
        painter.drawText(
            QRectF(origin.x() + 8.0, 6.0, 18.0, 18.0),
            Qt::AlignCenter,
            verticalLabel);
    }
    painter.restore();

    if (m_showAxisNumbers) {
        drawTickLabels(painter, niceStep(80.0 / m_scale));
    }
}

void Graph2DWidget::drawTickLabels(QPainter& painter, double step) const {
    const QPointF origin = toScreen(QPointF(0.0, 0.0));
    const bool horizontalVisible = origin.y() >= 0.0 && origin.y() <= height();
    const bool verticalVisible = origin.x() >= 0.0 && origin.x() <= width();

    const QPointF topLeft = toPlane(QPointF(0.0, 0.0));
    const QPointF bottomRight = toPlane(QPointF(width(), height()));
    const double xMin = std::min(topLeft.x(), bottomRight.x());
    const double xMax = std::max(topLeft.x(), bottomRight.x());
    const double yMin = std::min(topLeft.y(), bottomRight.y());
    const double yMax = std::max(topLeft.y(), bottomRight.y());

    painter.save();
    QFont tickFont = painter.font();
    tickFont.setPointSizeF(9.0);
    tickFont.setBold(false);
    painter.setFont(tickFont);
    painter.setPen(m_colors.mutedText);
    const QFontMetricsF metrics(tickFont);

    if (horizontalVisible) {
        const long long start = static_cast<long long>(std::ceil(xMin / step));
        const long long end = static_cast<long long>(std::floor(xMax / step));
        for (long long i = start; i <= end; ++i) {
            if (i == 0) {
                continue;
            }
            const double x = static_cast<double>(i) * step;
            const QPointF screen = toScreen(QPointF(x, 0.0));
            if (screen.x() < 4.0 || screen.x() > width() - 4.0) {
                continue;
            }
            painter.drawLine(
                QPointF(screen.x(), origin.y() - 3.0),
                QPointF(screen.x(), origin.y() + 3.0));

            const QString text = formatTick(x, step);
            const double textWidth = metrics.horizontalAdvance(text);
            const double labelY = (origin.y() + 16.0 < height() - 4.0) ? origin.y() + 5.0
                                                                      : origin.y() - 19.0;
            painter.drawText(
                QRectF(screen.x() - textWidth * 0.5 - 2.0, labelY, textWidth + 4.0, 14.0),
                Qt::AlignCenter,
                text);
        }
    }

    if (verticalVisible) {
        const long long start = static_cast<long long>(std::ceil(yMin / step));
        const long long end = static_cast<long long>(std::floor(yMax / step));
        for (long long i = start; i <= end; ++i) {
            if (i == 0) {
                continue;
            }
            const double y = static_cast<double>(i) * step;
            const QPointF screen = toScreen(QPointF(0.0, y));
            if (screen.y() < 4.0 || screen.y() > height() - 4.0) {
                continue;
            }
            painter.drawLine(
                QPointF(origin.x() - 3.0, screen.y()),
                QPointF(origin.x() + 3.0, screen.y()));

            const QString text = formatTick(y, step);
            const double textWidth = metrics.horizontalAdvance(text);
            const bool placeRight = origin.x() < textWidth + 12.0;
            const double labelX = placeRight ? origin.x() + 6.0 : origin.x() - textWidth - 10.0;
            painter.drawText(
                QRectF(labelX, screen.y() - 7.0, textWidth + 4.0, 14.0),
                Qt::AlignCenter,
                text);
        }
    }

    if (horizontalVisible && verticalVisible) {
        painter.drawText(QRectF(origin.x() + 4.0, origin.y() + 3.0, 20.0, 14.0),
                         Qt::AlignCenter,
                         QStringLiteral("0"));
    }
    painter.restore();
}

void Graph2DWidget::drawVectors(QPainter& painter) const {
    for (const GraphVector& vector : m_vectors) {
        if (vector.value.isZero()) {
            continue;
        }

        const QPointF start = toScreen(project(vector.tail));
        const QPointF tip = toScreen(project(vector.tail + vector.value));
        const QPointF delta = tip - start;
        const double length = std::hypot(delta.x(), delta.y());
        if (length < 1.0) {
            continue;
        }

        const QPointF direction = delta / length;
        const QPointF perpendicular(-direction.y(), direction.x());
        const QColor color = vector.color.isValid() ? vector.color : m_colors.accent;
        const double lineWidth = vector.selected ? 4.4 : 3.0;

        painter.save();
        painter.setRenderHint(QPainter::Antialiasing, true);

        if (vector.selected) {
            painter.setPen(QPen(m_colors.canvas, lineWidth + 4.0, Qt::SolidLine, Qt::RoundCap,
                                Qt::RoundJoin));
            painter.drawLine(start, tip);
        }

        QPen pen(color, lineWidth, vector.isResult ? Qt::DashLine : Qt::SolidLine, Qt::RoundCap,
                 Qt::RoundJoin);
        painter.setPen(pen);
        painter.drawLine(start, tip);
        drawArrowHead(painter, tip, direction, perpendicular, std::min(13.0, length * 0.28));

        if (vector.selected) {
            painter.setPen(QPen(m_colors.panel, 1.5));
            painter.setBrush(color);
            painter.drawEllipse(tip, 4.0, 4.0);
        }

        if (m_showLabels && !vector.name.isEmpty()) {
            QFont font = painter.font();
            font.setPointSizeF(10.0);
            font.setBold(vector.selected);
            painter.setFont(font);
            const QFontMetricsF metrics(font);
            const QString label = metrics.elidedText(vector.name, Qt::ElideRight, 190.0);

            const double textWidth = metrics.horizontalAdvance(label);
            const double textHeight = metrics.height();
            QPointF topLeft(tip.x() + 10.0, tip.y() - textHeight - 8.0);
            if (topLeft.x() + textWidth + 10.0 > width() - 4.0) {
                topLeft.setX(tip.x() - textWidth - 14.0);
            }
            if (topLeft.y() < 4.0) {
                topLeft.setY(tip.y() + 10.0);
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
            painter.drawText(
                QPointF(topLeft.x(), topLeft.y() + metrics.ascent()), label);
        }

        painter.restore();
    }
}
