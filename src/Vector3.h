#pragma once

#include <iosfwd>

class Vector3 {
public:
    double x{0.0};
    double y{0.0};
    double z{0.0};

    constexpr Vector3() noexcept = default;
    constexpr Vector3(double x_, double y_, double z_) noexcept : x(x_), y(y_), z(z_) {}

    static constexpr Vector3 zero() noexcept { return {}; }
    static constexpr Vector3 unitX() noexcept { return {1.0, 0.0, 0.0}; }
    static constexpr Vector3 unitY() noexcept { return {0.0, 1.0, 0.0}; }
    static constexpr Vector3 unitZ() noexcept { return {0.0, 0.0, 1.0}; }

    constexpr Vector3 operator+() const noexcept { return *this; }
    constexpr Vector3 operator-() const noexcept { return {-x, -y, -z}; }

    constexpr Vector3& operator+=(const Vector3& rhs) noexcept {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        return *this;
    }

    constexpr Vector3& operator-=(const Vector3& rhs) noexcept {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        return *this;
    }

    constexpr Vector3& operator*=(double scalar) noexcept {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return *this;
    }

    constexpr Vector3& operator/=(double scalar) noexcept {
        x /= scalar;
        y /= scalar;
        z /= scalar;
        return *this;
    }

    [[nodiscard]] constexpr double dot(const Vector3& rhs) const noexcept {
        return x * rhs.x + y * rhs.y + z * rhs.z;
    }

    [[nodiscard]] constexpr Vector3 cross(const Vector3& rhs) const noexcept {
        return {
            y * rhs.z - z * rhs.y,
            z * rhs.x - x * rhs.z,
            x * rhs.y - y * rhs.x,
        };
    }

    [[nodiscard]] constexpr double lengthSquared() const noexcept { return dot(*this); }
    [[nodiscard]] double length() const noexcept;

    [[nodiscard]] Vector3 normalized() const;
    Vector3& normalize();

    [[nodiscard]] double angleBetween(const Vector3& rhs) const;
    [[nodiscard]] bool isZero(double epsilon = 1e-12) const noexcept;
};

constexpr Vector3 operator+(Vector3 lhs, const Vector3& rhs) noexcept { return lhs += rhs; }
constexpr Vector3 operator-(Vector3 lhs, const Vector3& rhs) noexcept { return lhs -= rhs; }
constexpr Vector3 operator*(Vector3 vec, double scalar) noexcept { return vec *= scalar; }
constexpr Vector3 operator*(double scalar, Vector3 vec) noexcept { return vec *= scalar; }
constexpr Vector3 operator/(Vector3 vec, double scalar) noexcept { return vec /= scalar; }

constexpr bool operator==(const Vector3& lhs, const Vector3& rhs) noexcept {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

constexpr bool operator!=(const Vector3& lhs, const Vector3& rhs) noexcept {
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& os, const Vector3& vec);
