#include "Vector3.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <numbers>

namespace {

int failures = 0;

bool approx(double a, double b, double epsilon = 1e-9) {
    return std::abs(a - b) <= epsilon;
}

bool approxVec(const Vector3& a, const Vector3& b, double epsilon = 1e-9) {
    return approx(a.x, b.x, epsilon) && approx(a.y, b.y, epsilon) && approx(a.z, b.z, epsilon);
}

void check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

} // namespace

int main() {
    const Vector3 a(1.0, 2.0, 3.0);
    const Vector3 b(4.0, 5.0, 6.0);

    check(Vector3().isZero(), "default vector is zero");
    check(Vector3::unitX() == Vector3(1.0, 0.0, 0.0), "unitX");
    check(Vector3::unitY() == Vector3(0.0, 1.0, 0.0), "unitY");
    check(Vector3::unitZ() == Vector3(0.0, 0.0, 1.0), "unitZ");

    check(a + b == Vector3(5.0, 7.0, 9.0), "addition");
    check(a - b == Vector3(-3.0, -3.0, -3.0), "subtraction");
    check(-a == Vector3(-1.0, -2.0, -3.0), "unary minus");
    check(a * 2.0 == Vector3(2.0, 4.0, 6.0), "scalar multiplication");
    check(2.0 * a == Vector3(2.0, 4.0, 6.0), "scalar multiplication (commutative)");
    check(a / 2.0 == Vector3(0.5, 1.0, 1.5), "scalar division");

    check(approx(a.dot(b), 32.0), "dot product");
    check(approxVec(a.cross(b), Vector3(-3.0, 6.0, -3.0)), "cross product");
    check(approxVec(b.cross(a), Vector3(3.0, -6.0, 3.0)), "cross product anti-commutative");
    check(approx(a.lengthSquared(), 14.0), "length squared");
    check(approx(a.length(), std::sqrt(14.0)), "length");

    const Vector3 normalized = a.normalized();
    check(approx(normalized.length(), 1.0), "normalized length");
    check(approxVec(normalized, a / std::sqrt(14.0)), "normalized components");

    Vector3 mutableVec = a;
    mutableVec.normalize();
    check(approxVec(mutableVec, normalized), "normalize in place");

    Vector3 zero;
    check(zero.normalized().isZero(), "normalizing zero returns zero");
    zero.normalize();
    check(zero.isZero(), "in-place normalizing zero stays zero");

    check(approx(Vector3::unitX().angleBetween(Vector3::unitY()), std::numbers::pi / 2.0),
          "angle between orthogonal vectors");
    check(approx(Vector3::unitX().angleBetween(Vector3::unitX()), 0.0), "angle between parallel vectors");
    check(approx(Vector3::unitX().angleBetween(-Vector3::unitX()), std::numbers::pi),
          "angle between opposite vectors");
    check(approx(Vector3(1.0, 0.0, 0.0).angleBetween(Vector3(1.0, 1.0, 0.0)), std::numbers::pi / 4.0),
          "angle between 45 degree vectors");
    check(std::isnan(Vector3().angleBetween(Vector3::unitX())), "angle with zero vector is undefined");

    check(a != b, "inequality");
    check(a == Vector3(1.0, 2.0, 3.0), "equality");
    check(Vector3(1e-13, -1e-13, 1e-13).isZero(), "isZero with default epsilon");
    check(!Vector3(1e-6, 0.0, 0.0).isZero(), "isZero rejects larger components");

    if (failures != 0) {
        std::cerr << failures << " test(s) failed\n";
        return 1;
    }

    std::cout << "All Vector3 tests passed\n";
    return 0;
}
