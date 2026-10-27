#pragma once

#include "GraphTypes.h"

#include <QColor>
#include <QList>
#include <QString>

struct ThemeColors {
    QColor appBackground;
    QColor panel;
    QColor canvas;
    QColor border;
    QColor text;
    QColor mutedText;
    QColor accent;
    QColor accentSoft;
    QColor gridMinor;
    QColor gridMajor;
    QColor axis;
    QColor axisX;
    QColor axisY;
    QColor axisZ;
    QColor selection;
    QColor error;
    QColor result;
    QList<QColor> vectorColors;
};

namespace Theme {

ThemeColors colors(ThemeMode mode);
QString styleSheet(ThemeMode mode);

} // namespace Theme
