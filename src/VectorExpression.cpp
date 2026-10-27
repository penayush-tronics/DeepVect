#include "VectorExpression.h"

#include <QRegularExpression>
#include <QStringList>
#include <QVector>

#include <algorithm>
#include <cmath>
#include <numeric>

namespace {

constexpr double kEpsilon = 1e-12;

void skipSpaces(const QString& text, int& pos) {
    while (pos < text.size() && text.at(pos).isSpace()) {
        ++pos;
    }
}

bool parseNumberToken(const QString& text, int& pos, double& value) {
    const int start = pos;
    bool sawDigit = false;
    bool sawDot = false;

    while (pos < text.size()) {
        const QChar ch = text.at(pos);
        if (ch.isDigit()) {
            sawDigit = true;
            ++pos;
        } else if (ch == QLatin1Char('.') && !sawDot) {
            sawDot = true;
            ++pos;
        } else {
            break;
        }
    }

    if (!sawDigit) {
        pos = start;
        return false;
    }

    if (pos < text.size() && (text.at(pos) == QLatin1Char('e') || text.at(pos) == QLatin1Char('E'))) {
        const int exponentStart = pos;
        ++pos;
        if (pos < text.size() && (text.at(pos) == QLatin1Char('+') || text.at(pos) == QLatin1Char('-'))) {
            ++pos;
        }
        const int digitsStart = pos;
        while (pos < text.size() && text.at(pos).isDigit()) {
            ++pos;
        }
        if (pos == digitsStart) {
            pos = exponentStart;
        }
    }

    bool ok = false;
    value = text.mid(start, pos - start).toDouble(&ok);
    if (!ok) {
        pos = start;
        return false;
    }
    return true;
}


long long integerSqrt(long long value) {
    if (value <= 0) {
        return 0;
    }
    long long root = static_cast<long long>(std::sqrt(static_cast<long double>(value)));
    while (root + 1 <= value / (root + 1)) {
        ++root;
    }
    while (root > 0 && root > value / root) {
        --root;
    }
    return root;
}

struct IntegerComponents {
    long long x = 0;
    long long y = 0;
    long long z = 0;
};

bool scaleToIntegerComponents(const Vector3& value, IntegerComponents& out) {
    static constexpr long long scales[] = {
        1LL, 10LL, 100LL, 1000LL, 10000LL, 100000LL, 1000000LL,
    };

    for (const long long scale : scales) {
        const double scaledX = value.x * static_cast<double>(scale);
        const double scaledY = value.y * static_cast<double>(scale);
        const double scaledZ = value.z * static_cast<double>(scale);
        if (!std::isfinite(scaledX) || !std::isfinite(scaledY) || !std::isfinite(scaledZ)
            || std::abs(scaledX) > 9.0e18 || std::abs(scaledY) > 9.0e18
            || std::abs(scaledZ) > 9.0e18) {
            continue;
        }

        const long long x = std::llround(scaledX);
        const long long y = std::llround(scaledY);
        const long long z = std::llround(scaledZ);
        const auto close = [](double exact, long long rounded) {
            return std::abs(exact - static_cast<double>(rounded))
                   <= 1e-7 * std::max(1.0, std::abs(exact));
        };
        if (!close(scaledX, x) || !close(scaledY, y) || !close(scaledZ, z)) {
            continue;
        }

        if (x == 0 && y == 0 && z == 0) {
            return false;
        }
        out = {x, y, z};
        return true;
    }

    return false;
}

QString fractionMagnitude(long long magnitude, long long denominator, bool radical) {
    QString result = QString::number(magnitude);
    if (radical || denominator != 1) {
        result += QLatin1Char('/');
        if (radical) {
            result += QStringLiteral("\u221A");
        }
        result += QString::number(denominator);
    }
    return result;
}

} // namespace

namespace VectorExpression {

QString formatNumber(double value) {
    if (std::abs(value) < kEpsilon) {
        return QStringLiteral("0");
    }
    return QString::number(value, 'g', 6);
}

QString formatUnitFraction(const Vector3& value, bool column) {
    IntegerComponents components;
    if (!scaleToIntegerComponents(value, components)) {
        const Vector3 unit = value.normalized();
        if (column) {
            return QStringLiteral("[%1, %2, %3]")
                .arg(formatNumber(unit.x), formatNumber(unit.y), formatNumber(unit.z));
        }
        return formatIJK(unit);
    }

    const long long denominatorSquared =
        components.x * components.x + components.y * components.y
        + components.z * components.z;
    if (denominatorSquared <= 0) {
        return QStringLiteral("0");
    }

    long long denominator = integerSqrt(denominatorSquared);
    bool radical = denominator * denominator != denominatorSquared;
    if (radical) {
        long long bestSquare = 1;
        for (long long candidate = denominator; candidate >= 2; --candidate) {
            if (denominatorSquared % (candidate * candidate) != 0) {
                continue;
            }
            if (components.x % candidate == 0 && components.y % candidate == 0
                && components.z % candidate == 0) {
                bestSquare = candidate;
                break;
            }
        }

        if (bestSquare > 1) {
            components.x /= bestSquare;
            components.y /= bestSquare;
            components.z /= bestSquare;
            denominator = denominatorSquared / (bestSquare * bestSquare);
            radical = denominator != 1;
        } else {
            denominator = denominatorSquared;
        }
    } else if (denominator > 0) {
        const long long common = std::gcd(
            std::gcd(std::abs(components.x), std::abs(components.y)),
            std::gcd(std::abs(components.z), denominator));
        if (common > 1) {
            components.x /= common;
            components.y /= common;
            components.z /= common;
            denominator /= common;
        }
    }

    const long long values[3] = {components.x, components.y, components.z};
    static const char* const names[3] = {"i", "j", "k"};

    if (column) {
        QStringList parts;
        for (const long long component : values) {
            if (component == 0) {
                parts.append(QStringLiteral("0"));
            } else {
                const bool negative = component < 0;
                parts.append((negative ? QStringLiteral("-") : QString())
                             + fractionMagnitude(std::abs(component), denominator, radical));
            }
        }
        return QStringLiteral("[%1]").arg(parts.join(QStringLiteral(", ")));
    }

    QString result;
    bool first = true;
    for (int i = 0; i < 3; ++i) {
        const long long component = values[i];
        if (component == 0) {
            continue;
        }

        const bool negative = component < 0;
        const long long magnitude = std::abs(component);
        const bool wholeCoefficient = !radical && magnitude == denominator;

        if (first) {
            if (negative) {
                result += QLatin1Char('-');
            }
        } else {
            result += negative ? QStringLiteral(" - ") : QStringLiteral(" + ");
        }

        if (!wholeCoefficient) {
            QString coefficient = fractionMagnitude(magnitude, denominator, radical);
            if (radical || denominator != 1) {
                coefficient = QLatin1Char('(') + coefficient + QLatin1Char(')');
            }
            result += coefficient;
        }
        result += QString::fromLatin1(names[i]);
        first = false;
    }

    return first ? QStringLiteral("0") : result;
}

QString formatIJK(const Vector3& value) {
    struct Axis {
        double component;
        const char* name;
    };

    const Axis axes[3] = {
        {value.x, "i"},
        {value.y, "j"},
        {value.z, "k"},
    };

    QString result;
    bool first = true;

    for (const Axis& axis : axes) {
        if (std::abs(axis.component) < kEpsilon) {
            continue;
        }

        const bool negative = axis.component < 0.0;
        const double magnitude = std::abs(axis.component);

        if (first) {
            if (negative) {
                result += QLatin1Char('-');
            }
        } else {
            result += negative ? QStringLiteral(" - ") : QStringLiteral(" + ");
        }

        if (std::abs(magnitude - 1.0) > kEpsilon) {
            result += formatNumber(magnitude);
        }
        result += QString::fromLatin1(axis.name);
        first = false;
    }

    return first ? QStringLiteral("0") : result;
}

std::optional<Vector3> parseVector(const QString& text, QString* error) {
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        if (error != nullptr) {
            *error = QStringLiteral("Enter a vector expression.");
        }
        return std::nullopt;
    }

    const QChar firstChar = trimmed.front();
    if (firstChar == QLatin1Char('[') || firstChar == QLatin1Char('(')) {
        const QChar expectedClose = firstChar == QLatin1Char('[') ? QLatin1Char(']') : QLatin1Char(')');
        if (trimmed.back() != expectedClose) {
            if (error != nullptr) {
                *error = QStringLiteral("Missing closing '%1'.").arg(expectedClose);
            }
            return std::nullopt;
        }

        const QString inner = trimmed.mid(1, trimmed.size() - 2);
        const QStringList parts = inner.split(QLatin1Char(','));
        if (parts.size() != 3) {
            if (error != nullptr) {
                *error = QStringLiteral("Column input needs exactly three numbers.");
            }
            return std::nullopt;
        }

        double components[3] = {0.0, 0.0, 0.0};
        for (int i = 0; i < 3; ++i) {
            bool ok = false;
            components[i] = parts.at(i).trimmed().toDouble(&ok);
            if (!ok) {
                if (error != nullptr) {
                    *error = QStringLiteral("'%1' is not a valid number.").arg(parts.at(i).trimmed());
                }
                return std::nullopt;
            }
        }
        return Vector3(components[0], components[1], components[2]);
    }

    Vector3 result;
    int pos = 0;
    int termIndex = 0;

    while (true) {
        skipSpaces(trimmed, pos);
        if (pos >= trimmed.size()) {
            if (termIndex == 0) {
                if (error != nullptr) {
                    *error = QStringLiteral("Enter a vector expression.");
                }
                return std::nullopt;
            }
            break;
        }

        double sign = 1.0;
        if (trimmed.at(pos) == QLatin1Char('+')) {
            ++pos;
            skipSpaces(trimmed, pos);
        } else if (trimmed.at(pos) == QLatin1Char('-')) {
            sign = -1.0;
            ++pos;
            skipSpaces(trimmed, pos);
        }

        double coefficient = 1.0;
        bool hadNumber = false;

        if (pos < trimmed.size()) {
            double parsedNumber = 0.0;
            if (parseNumberToken(trimmed, pos, parsedNumber)) {
                coefficient = parsedNumber;
                hadNumber = true;
                skipSpaces(trimmed, pos);
            }
        }

        if (hadNumber && pos < trimmed.size() && trimmed.at(pos) == QLatin1Char('*')) {
            ++pos;
            skipSpaces(trimmed, pos);
        }

        int axisIndex = -1;
        if (pos < trimmed.size()) {
            const QChar axisChar = trimmed.at(pos);
            if (axisChar == QLatin1Char('i')) {
                axisIndex = 0;
                ++pos;
            } else if (axisChar == QLatin1Char('j')) {
                axisIndex = 1;
                ++pos;
            } else if (axisChar == QLatin1Char('k')) {
                axisIndex = 2;
                ++pos;
            }
        }

        if (axisIndex < 0) {
            if (hadNumber && std::abs(sign * coefficient) < kEpsilon) {
                skipSpaces(trimmed, pos);
                if (pos < trimmed.size() && trimmed.at(pos) != QLatin1Char('+')
                    && trimmed.at(pos) != QLatin1Char('-')) {
                    if (error != nullptr) {
                        *error = QStringLiteral("Expected '+' or '-' between terms.");
                    }
                    return std::nullopt;
                }
            } else {
                if (error != nullptr) {
                    *error = QStringLiteral("Each term must use i, j, or k.");
                }
                return std::nullopt;
            }
        } else {
            const double contribution = sign * coefficient;
            if (axisIndex == 0) {
                result.x += contribution;
            } else if (axisIndex == 1) {
                result.y += contribution;
            } else {
                result.z += contribution;
            }
        }

        ++termIndex;
        skipSpaces(trimmed, pos);

        if (pos >= trimmed.size()) {
            break;
        }

        const QChar next = trimmed.at(pos);
        if (next == QLatin1Char('+') || next == QLatin1Char('-')) {
            continue;
        }

        if (error != nullptr) {
            *error = QStringLiteral("Expected '+' or '-' between terms.");
        }
        return std::nullopt;
    }

    return result;
}

namespace {

enum class TokenType {
    End,
    Identifier,
    Number,
    Plus,
    Minus,
    Dot,
    Cross,
    LParen,
    RParen,
    Comma,
    Invalid,
};

struct Token {
    TokenType type = TokenType::End;
    QString text;
    int position = 0;
};

QVector<Token> tokenize(const QString& text, QString& error) {
    QVector<Token> tokens;
    int pos = 0;

    while (pos < text.size()) {
        const QChar ch = text.at(pos);

        if (ch.isSpace()) {
            ++pos;
            continue;
        }

        if (ch.isLetter() || ch == QLatin1Char('_')) {
            const int start = pos;
            ++pos;
            while (pos < text.size()) {
                const QChar next = text.at(pos);
                if (!next.isLetterOrNumber() && next != QLatin1Char('_')) {
                    break;
                }
                ++pos;
            }
            tokens.append({TokenType::Identifier, text.mid(start, pos - start), start});
            continue;
        }

        if (ch.isDigit()) {
            const int start = pos;
            while (pos < text.size() && (text.at(pos).isDigit() || text.at(pos) == QLatin1Char('.'))) {
                ++pos;
            }
            tokens.append({TokenType::Number, text.mid(start, pos - start), start});
            continue;
        }

        TokenType type = TokenType::Invalid;
        switch (ch.unicode()) {
        case '+':
            type = TokenType::Plus;
            break;
        case '-':
            type = TokenType::Minus;
            break;
        case '.':
        case '*':
        case 0x00B7: // middle dot
            type = TokenType::Dot;
            break;
        case 0x00D7: // multiplication sign
            type = TokenType::Cross;
            break;
        case '(':
            type = TokenType::LParen;
            break;
        case ')':
            type = TokenType::RParen;
            break;
        case ',':
            type = TokenType::Comma;
            break;
        default:
            break;
        }

        if (type == TokenType::Invalid) {
            error = QStringLiteral("Unexpected character '%1' at position %2.").arg(ch).arg(pos + 1);
            return {};
        }

        tokens.append({type, QString(ch), pos});
        ++pos;
    }

    tokens.append({TokenType::End, QString(), static_cast<int>(text.size())});
    return tokens;
}

class OperationParser {
public:
    OperationParser(QVector<Token> tokens, const QHash<QString, Vector3>& vectors)
        : m_tokens(std::move(tokens)), m_vectors(vectors) {}

    OperationResult parse() {
        OperationResult result;

        if (m_tokens.size() == 1 && m_tokens.first().type == TokenType::End) {
            return result;
        }

        ExpressionValue value;
        if (!parseExpression(value)) {
            result.error = m_error;
            return result;
        }

        if (peek().type != TokenType::End) {
            result.error = QStringLiteral("Unexpected token '%1' at position %2.")
                               .arg(peek().text)
                               .arg(peek().position + 1);
            return result;
        }

        result.valid = true;
        result.value = value;
        return result;
    }

private:
    const Token& peek() const { return m_tokens.at(m_pos); }

    Token advance() {
        const Token token = m_tokens.at(m_pos);
        if (m_pos < m_tokens.size() - 1) {
            ++m_pos;
        }
        return token;
    }

    bool isCrossOperator(const Token& token) const {
        return token.type == TokenType::Cross
               || (token.type == TokenType::Identifier
                   && (token.text == QStringLiteral("x") || token.text == QStringLiteral("X")));
    }

    bool expect(TokenType type, const QString& description) {
        if (peek().type != type) {
            m_error = QStringLiteral("Expected %1 at position %2.")
                          .arg(description)
                          .arg(peek().position + 1);
            return false;
        }
        advance();
        return true;
    }

    bool requireVector(const ExpressionValue& value, const QString& operation) {
        if (value.type != ExpressionValueType::Vector) {
            m_error = QStringLiteral("%1 requires vector operands.").arg(operation);
            return false;
        }
        return true;
    }

    bool parseExpression(ExpressionValue& out) {
        if (!parseTerm(out)) {
            return false;
        }

        while (peek().type == TokenType::Plus || peek().type == TokenType::Minus) {
            const Token op = advance();
            ExpressionValue rhs;
            if (!parseTerm(rhs)) {
                return false;
            }
            if (!requireVector(out, QStringLiteral("Addition/subtraction"))
                || !requireVector(rhs, QStringLiteral("Addition/subtraction"))) {
                return false;
            }

            if (op.type == TokenType::Plus) {
                out = ExpressionValue::fromVector(out.vector + rhs.vector);
            } else {
                out = ExpressionValue::fromVector(out.vector - rhs.vector);
            }
        }

        return true;
    }

    bool parseTerm(ExpressionValue& out) {
        if (!parsePrimary(out)) {
            return false;
        }

        while (peek().type == TokenType::Dot || isCrossOperator(peek())) {
            const Token op = advance();
            ExpressionValue rhs;
            if (!parsePrimary(rhs)) {
                return false;
            }

            const QString operation = op.type == TokenType::Dot
                                          ? QStringLiteral("The dot product")
                                          : QStringLiteral("The cross product");
            if (!requireVector(out, operation) || !requireVector(rhs, operation)) {
                return false;
            }

            if (op.type == TokenType::Dot) {
                out = ExpressionValue::fromScalar(out.vector.dot(rhs.vector));
            } else {
                out = ExpressionValue::fromVector(out.vector.cross(rhs.vector));
            }
        }

        return true;
    }

    bool parsePrimary(ExpressionValue& out) {
        const Token token = peek();

        if (token.type == TokenType::Identifier) {
            advance();

            if (token.text == QStringLiteral("x") || token.text == QStringLiteral("X")) {
                m_error = QStringLiteral("'%1' is reserved for the cross product. Use A x B.")
                              .arg(token.text);
                return false;
            }

            if (token.text == QStringLiteral("dot") || token.text == QStringLiteral("cross")) {
                m_error = QStringLiteral("Use '.' for dot and 'x' for cross, e.g. A . B or A x B.");
                return false;
            }

            const auto it = m_vectors.constFind(token.text);
            if (it == m_vectors.constEnd()) {
                m_error = QStringLiteral("Unknown vector '%1' at position %2.")
                              .arg(token.text)
                              .arg(token.position + 1);
                return false;
            }

            out = ExpressionValue::fromVector(it.value());
            return true;
        }

        if (token.type == TokenType::LParen) {
            advance();
            if (!parseExpression(out)) {
                return false;
            }
            return expect(TokenType::RParen, QStringLiteral("')'"));
        }

        if (token.type == TokenType::Number) {
            m_error = QStringLiteral("Numeric literals are not supported in operation equations.");
            return false;
        }

        if (token.type == TokenType::End) {
            m_error = QStringLiteral("Unexpected end of expression.");
            return false;
        }

        m_error = QStringLiteral("Expected a vector name or '(' at position %1.")
                      .arg(token.position + 1);
        return false;
    }

    QVector<Token> m_tokens;
    QHash<QString, Vector3> m_vectors;
    int m_pos = 0;
    QString m_error;
};

} // namespace

OperationResult evaluate(const QString& expression, const QHash<QString, Vector3>& vectors) {
    const QString trimmed = expression.trimmed();
    if (trimmed.isEmpty()) {
        return {};
    }

    QString tokenError;
    QVector<Token> tokens = tokenize(trimmed, tokenError);
    if (!tokenError.isEmpty()) {
        OperationResult result;
        result.error = tokenError;
        return result;
    }

    OperationParser parser(std::move(tokens), vectors);
    return parser.parse();
}

QList<AdditiveTerm> additiveTerms(const QString& expression,
                                  const QHash<QString, Vector3>& vectors) {
    QList<AdditiveTerm> terms;
    const QString trimmed = expression.trimmed();
    if (trimmed.isEmpty()) {
        return terms;
    }

    QString tokenError;
    const QVector<Token> tokens = tokenize(trimmed, tokenError);
    if (!tokenError.isEmpty()) {
        return {};
    }

    int depth = 0;
    int start = 0;
    double sign = 1.0;

    const auto appendTerm = [&](int endPosition) {
        const QString termText = trimmed.mid(start, endPosition - start).trimmed();
        if (termText.isEmpty()) {
            return false;
        }

        const OperationResult result = evaluate(termText, vectors);
        if (!result.valid || result.value.type != ExpressionValueType::Vector) {
            return false;
        }

        AdditiveTerm term;
        term.sign = sign;
        term.text = termText;
        term.value = result.value.vector;

        QString termTokenError;
        const QVector<Token> termTokens = tokenize(termText, termTokenError);
        if (termTokenError.isEmpty() && termTokens.size() == 2
            && termTokens.first().type == TokenType::Identifier
            && termTokens.first().text != QStringLiteral("x")
            && termTokens.first().text != QStringLiteral("X")
            && termTokens.first().text != QStringLiteral("dot")
            && termTokens.first().text != QStringLiteral("cross")) {
            term.plainName = termTokens.first().text;
        }

        terms.append(term);
        return true;
    };

    for (const Token& token : tokens) {
        if (token.type == TokenType::End) {
            break;
        }

        if (token.type == TokenType::LParen) {
            ++depth;
        } else if (token.type == TokenType::RParen) {
            --depth;
            if (depth < 0) {
                return {};
            }
        }

        if (depth == 0 && (token.type == TokenType::Plus || token.type == TokenType::Minus)) {
            if (!appendTerm(token.position)) {
                return {};
            }
            sign = token.type == TokenType::Plus ? 1.0 : -1.0;
            start = token.position + 1;
        }
    }

    if (depth != 0 || !appendTerm(trimmed.size())) {
        return {};
    }

    return terms;
}

} // namespace VectorExpression
