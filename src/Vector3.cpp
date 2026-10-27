#include "Vector3.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <ostream>

double Vector3::length() const noexcept {
    return std::sqrt(lengthSquared());
}

Vector3 Vector3::normalized() const {
    const double len = length();
    if (len < 1e-12) {
        return {};
    }
    return *this / len;
}

Vector3& Vector3::normalize() {
    const double len = length();
    if (len < 1e-12) {
        x = 0.0;
        y = 0.0;
        z = 0.0;
        return *this;
    }
    x /= len;
    y /= len;
    z /= len;
    return *this;
}

double Vector3::angleBetween(const Vector3& rhs) const {
    const double lenA = length();
    const double lenB = rhs.length();
    if (lenA < 1e-12 || lenB < 1e-12) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    double cosine = dot(rhs) / (lenA * lenB);
    cosine = std::clamp(cosine, -1.0, 1.0);
    return std::acos(cosine);
}

bool Vector3::isZero(double epsilon) const noexcept {
    return std::abs(x) <= epsilon && std::abs(y) <= epsilon && std::abs(z) <= epsilon;
}

std::ostream& operator<<(std::ostream& os, const Vector3& vec) {
    os << '(' << vec.x << ", " << vec.y << ", " << vec.z << ')';
    return os;
}
