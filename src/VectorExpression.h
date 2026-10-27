#pragma once

#include "Vector3.h"

#include <QHash>
#include <QList>
#include <QString>

#include <optional>

enum class ExpressionValueType {
    Vector,
    Scalar,
};

struct ExpressionValue {
    ExpressionValueType type = ExpressionValueType::Vector;
    Vector3 vector;
    double scalar = 0.0;

    static ExpressionValue fromVector(const Vector3& value) {
        ExpressionValue result;
        result.type = ExpressionValueType::Vector;
        result.vector = value;
        return result;
    }

    static ExpressionValue fromScalar(double value) {
        ExpressionValue result;
        result.type = ExpressionValueType::Scalar;
        result.scalar = value;
        return result;
    }
};

struct OperationResult {
    bool valid = false;
    QString error;
    ExpressionValue value;
};

struct AdditiveTerm {
    double sign = 1.0;
    QString text;
    QString plainName;
    Vector3 value;
};

namespace VectorExpression {

std::optional<Vector3> parseVector(const QString& text, QString* error = nullptr);
QString formatNumber(double value);
QString formatIJK(const Vector3& value);
QString formatUnitFraction(const Vector3& value, bool column = false);
OperationResult evaluate(const QString& expression, const QHash<QString, Vector3>& vectors);
QList<AdditiveTerm> additiveTerms(const QString& expression,
                                  const QHash<QString, Vector3>& vectors);

} // namespace VectorExpression
