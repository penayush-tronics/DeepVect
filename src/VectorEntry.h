#pragma once

#include "Vector3.h"

#include <QString>

struct VectorEntry {
    int id = 0;
    QString name;
    Vector3 value;
    int colorIndex = 0;
    bool visible = true;
};
