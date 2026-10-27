#include "VectorExpression.h"

#include <QHash>
#include <QString>

#include <cmath>
#include <iostream>

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

void checkVector(const QString& input, const Vector3& expected, const char* message) {
    QString error;
    const auto parsed = VectorExpression::parseVector(input, &error);
    check(parsed.has_value(), message);
    if (parsed.has_value()) {
        check(approxVec(*parsed, expected), message);
    } else {
        std::cerr << "  input: " << input.toStdString() << " error: " << error.toStdString() << '\n';
    }
}

void checkInvalidVector(const QString& input, const char* message) {
    QString error;
    const auto parsed = VectorExpression::parseVector(input, &error);
    check(!parsed.has_value(), message);
}

void checkVectorResult(
    const QString& expression,
    const QHash<QString, Vector3>& vectors,
    const Vector3& expected,
    const char* message) {
    const OperationResult result = VectorExpression::evaluate(expression, vectors);
    check(result.valid, message);
    if (!result.valid) {
        std::cerr << "  expression: " << expression.toStdString()
                  << " error: " << result.error.toStdString() << '\n';
        return;
    }
    check(result.value.type == ExpressionValueType::Vector, message);
    check(approxVec(result.value.vector, expected), message);
}

void checkScalarResult(
    const QString& expression,
    const QHash<QString, Vector3>& vectors,
    double expected,
    const char* message) {
    const OperationResult result = VectorExpression::evaluate(expression, vectors);
    check(result.valid, message);
    if (!result.valid) {
        std::cerr << "  expression: " << expression.toStdString()
                  << " error: " << result.error.toStdString() << '\n';
        return;
    }
    check(result.value.type == ExpressionValueType::Scalar, message);
    check(approx(result.value.scalar, expected), message);
}

void checkInvalidOperation(
    const QString& expression,
    const QHash<QString, Vector3>& vectors,
    const char* message) {
    const OperationResult result = VectorExpression::evaluate(expression, vectors);
    check(!result.valid, message);
    if (result.valid) {
        std::cerr << "  expression unexpectedly valid: " << expression.toStdString() << '\n';
    }
}

} // namespace

int main() {
    checkVector(QStringLiteral("3i + 2j - k"), Vector3(3.0, 2.0, -1.0), "vector ijk parsing");
    checkVector(QStringLiteral("-2.5j + 0.5k"), Vector3(0.0, -2.5, 0.5), "vector signed decimal parsing");
    checkVector(QStringLiteral("i - j + 4k"), Vector3(1.0, -1.0, 4.0), "vector unit coefficients");
    checkVector(QStringLiteral("0"), Vector3(), "vector zero");
    checkVector(QStringLiteral("2 * i + 3 j"), Vector3(2.0, 3.0, 0.0), "vector optional star and spaces");
    checkVector(QStringLiteral("[1, 2, 3]"), Vector3(1.0, 2.0, 3.0), "bracket column parsing");
    checkVector(QStringLiteral("(1, -2, 3.5)"), Vector3(1.0, -2.0, 3.5), "paren column parsing");

    checkInvalidVector(QStringLiteral(""), "empty vector rejected");
    checkInvalidVector(QStringLiteral("3i +"), "trailing vector operator rejected");
    checkInvalidVector(QStringLiteral("2"), "bare vector number rejected");
    checkInvalidVector(QStringLiteral("ai + 2j"), "unknown vector symbol rejected");
    checkInvalidVector(QStringLiteral("3i + 2j +"), "trailing plus rejected");
    checkInvalidVector(QStringLiteral("[1,2]"), "short column vector rejected");

    check(VectorExpression::formatIJK(Vector3(3.0, 2.0, 1.0)) == QStringLiteral("3i + 2j + k"),
          "format ijk positive");
    check(VectorExpression::formatIJK(Vector3(0.0, -1.0, 0.0)) == QStringLiteral("-j"),
          "format ijk negative unit");
    check(VectorExpression::formatIJK(Vector3(-1.0, 0.0, 2.5)) == QStringLiteral("-i + 2.5k"),
          "format ijk mixed");
    check(VectorExpression::formatIJK(Vector3()) == QStringLiteral("0"), "format ijk zero");

    check(VectorExpression::formatUnitFraction(Vector3(3.0, 2.0, 1.0))
              == QStringLiteral("(3/\u221A14)i + (2/\u221A14)j + (1/\u221A14)k"),
          "unit fraction radical denominator");
    check(VectorExpression::formatUnitFraction(Vector3(3.0, 4.0, 0.0))
              == QStringLiteral("(3/5)i + (4/5)j"),
          "unit fraction integer denominator");
    check(VectorExpression::formatUnitFraction(Vector3(6.0, 8.0, 0.0))
              == QStringLiteral("(3/5)i + (4/5)j"),
          "unit fraction reduces common factor");
    check(VectorExpression::formatUnitFraction(Vector3(0.25, 0.5, 0.0))
              == QStringLiteral("(1/\u221A5)i + (2/\u221A5)j"),
          "unit fraction simplifies decimal components");
    check(VectorExpression::formatUnitFraction(Vector3(0.0, -1.0, 0.0))
              == QStringLiteral("-j"),
          "unit fraction omits whole coefficient");
    check(VectorExpression::formatUnitFraction(Vector3(3.0, 2.0, 1.0), true)
              == QStringLiteral("[3/\u221A14, 2/\u221A14, 1/\u221A14]"),
          "unit fraction column format");

    QHash<QString, Vector3> vectors;
    vectors.insert(QStringLiteral("A"), Vector3(1.0, 2.0, 3.0));
    vectors.insert(QStringLiteral("B"), Vector3(4.0, 5.0, 6.0));
    vectors.insert(QStringLiteral("C"), Vector3(1.0, 0.0, 0.0));

    checkVectorResult(QStringLiteral("A + B"), vectors, Vector3(5.0, 7.0, 9.0), "operation addition");
    checkVectorResult(QStringLiteral("A - B"), vectors, Vector3(-3.0, -3.0, -3.0), "operation subtraction");
    checkVectorResult(QStringLiteral("A + B - C"), vectors, Vector3(4.0, 7.0, 9.0), "chained addition");
    checkScalarResult(QStringLiteral("A . B"), vectors, 32.0, "dot period syntax");
    checkScalarResult(QStringLiteral("A.B"), vectors, 32.0, "dot adjacent period syntax");
    checkScalarResult(QStringLiteral("A · B"), vectors, 32.0, "dot infix symbol");
    checkScalarResult(QStringLiteral("A * B"), vectors, 32.0, "dot star syntax");
    checkVectorResult(QStringLiteral("A x B"), vectors, Vector3(-3.0, 6.0, -3.0), "cross x syntax");
    checkVectorResult(QStringLiteral("A X B"), vectors, Vector3(-3.0, 6.0, -3.0), "cross uppercase x syntax");
    checkVectorResult(QStringLiteral("A × B"), vectors, Vector3(-3.0, 6.0, -3.0), "cross infix symbol");
    checkVectorResult(
        QStringLiteral("(A + B) x C"),
        vectors,
        Vector3(0.0, 9.0, -7.0),
        "cross with parenthesized left side");
    checkVectorResult(
        QStringLiteral("A x (B + C)"),
        vectors,
        Vector3(-3.0, 9.0, -5.0),
        "cross with parenthesized right side");
    checkScalarResult(
        QStringLiteral("(A x B) . C"),
        vectors,
        -3.0,
        "dot after parenthesized cross");
    checkScalarResult(
        QStringLiteral("A × B · C"),
        vectors,
        -3.0,
        "cross then dot precedence");
    checkInvalidOperation(QStringLiteral("A . B x C"), vectors, "dot then cross rejected");

    checkInvalidOperation(QStringLiteral("A +"), vectors, "incomplete operation rejected");
    checkInvalidOperation(QStringLiteral("A + 2"), vectors, "numeric literal rejected");
    checkInvalidOperation(QStringLiteral("A . B + C"), vectors, "scalar plus vector rejected");
    checkInvalidOperation(QStringLiteral("A · B × C"), vectors, "scalar cross rejected");
    checkInvalidOperation(QStringLiteral("A + D"), vectors, "unknown vector rejected");
    checkInvalidOperation(QStringLiteral("dot(A, B)"), vectors, "function syntax removed");
    checkInvalidOperation(QStringLiteral("A x"), vectors, "incomplete cross rejected");
    checkInvalidOperation(QStringLiteral("x + A"), vectors, "reserved x name rejected");

    const QList<AdditiveTerm> chained =
        VectorExpression::additiveTerms(QStringLiteral("A + B x C"), vectors);
    check(chained.size() == 2, "additive terms split at top level");
    if (chained.size() == 2) {
        check(chained.at(0).plainName == QStringLiteral("A"), "plain additive term detected");
        check(chained.at(1).plainName.isEmpty(), "composite additive term detected");
        check(chained.at(1).sign == 1.0, "positive composite term sign");
        check(approxVec(chained.at(1).value, Vector3(0.0, 6.0, -5.0)),
              "composite term evaluated with precedence");
    }

    const QList<AdditiveTerm> subtracted =
        VectorExpression::additiveTerms(QStringLiteral("A - (B + C)"), vectors);
    check(subtracted.size() == 2, "parenthesized additive terms split");
    if (subtracted.size() == 2) {
        check(subtracted.at(1).sign == -1.0, "negative additive term sign");
        check(approxVec(subtracted.at(1).value, Vector3(5.0, 5.0, 6.0)),
              "parenthesized additive term value");
    }

    const QList<AdditiveTerm> grouped =
        VectorExpression::additiveTerms(QStringLiteral("(A + B) x C"), vectors);
    check(grouped.size() == 1, "grouped cross stays one additive term");
    check(VectorExpression::additiveTerms(QStringLiteral("A . B + C"), vectors).isEmpty(),
          "scalar additive term is not chainable");

    const OperationResult empty = VectorExpression::evaluate(QStringLiteral(""), vectors);
    check(!empty.valid && empty.error.isEmpty(), "empty operation is neutral");

    if (failures != 0) {
        std::cerr << failures << " test(s) failed\n";
        return 1;
    }

    std::cout << "All expression tests passed\n";
    return 0;
}
