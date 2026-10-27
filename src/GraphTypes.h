#pragma once

#include "Vector3.h"

#include <QColor>
#include <QString>

enum class InputFormat {
    IJK,
    Column,
};

enum class ViewMode {
    TwoD,
    ThreeD,
};

enum class Plane2D {
    XY,
    XZ,
    YZ,
};

enum class ThemeMode {
    Light,
    Dark,
};

enum class ArrowMode {
    Origin,
    HeadToTail,
};

struct GraphVector {
    int id = -1;
    QString name;
    Vector3 tail;
    Vector3 value;
    QColor color;
    bool selected = false;
    bool isResult = false;
};
