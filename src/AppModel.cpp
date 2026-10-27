#include "AppModel.h"

#include "Theme.h"

#include <QRegularExpression>
#include <QSet>

namespace {

bool isValidName(const QString& name, QString* error) {
    static const QRegularExpression pattern(QStringLiteral("^[A-Za-z_][A-Za-z0-9_]*$"));
    if (!pattern.match(name).hasMatch()) {
        if (error != nullptr) {
            *error = QStringLiteral("Names must start with a letter or underscore and contain only letters, digits, or underscores.");
        }
        return false;
    }

    if (name == QStringLiteral("i") || name == QStringLiteral("j") || name == QStringLiteral("k")
        || name == QStringLiteral("x") || name == QStringLiteral("X")
        || name == QStringLiteral("dot") || name == QStringLiteral("cross")) {
        if (error != nullptr) {
            *error = QStringLiteral("'%1' is reserved. Choose another name.").arg(name);
        }
        return false;
    }

    return true;
}

} // namespace

AppModel::AppModel(QObject* parent)
    : QObject(parent) {
    addVector(QStringLiteral("A"), Vector3(3.0, 2.0, 1.0));
    addVector(QStringLiteral("B"), Vector3(-1.0, 4.0, 2.0));
    m_operationExpression = QStringLiteral("A + B");
    if (!m_vectors.isEmpty()) {
        m_selectedId = m_vectors.first().id;
    }
}

const QList<VectorEntry>& AppModel::vectors() const {
    return m_vectors;
}

int AppModel::selectedId() const {
    return m_selectedId;
}

const VectorEntry* AppModel::vectorById(int id) const {
    for (const VectorEntry& entry : m_vectors) {
        if (entry.id == id) {
            return &entry;
        }
    }
    return nullptr;
}

VectorEntry* AppModel::vectorByIdMutable(int id) {
    for (VectorEntry& entry : m_vectors) {
        if (entry.id == id) {
            return &entry;
        }
    }
    return nullptr;
}

bool AppModel::hasName(const QString& name, int exceptId) const {
    for (const VectorEntry& entry : m_vectors) {
        if (entry.id != exceptId && entry.name == name) {
            return true;
        }
    }
    return false;
}

InputFormat AppModel::inputFormat() const {
    return m_inputFormat;
}

ViewMode AppModel::viewMode() const {
    return m_viewMode;
}

Plane2D AppModel::plane2D() const {
    return m_plane2D;
}

ThemeMode AppModel::themeMode() const {
    return m_themeMode;
}

bool AppModel::showAxisNumbers() const {
    return m_showAxisNumbers;
}

bool AppModel::showLabels() const {
    return m_showLabels;
}

ArrowMode AppModel::arrowMode() const {
    return m_arrowMode;
}

QString AppModel::operationExpression() const {
    return m_operationExpression;
}

QHash<QString, Vector3> AppModel::namedVectors() const {
    QHash<QString, Vector3> result;
    for (const VectorEntry& entry : m_vectors) {
        result.insert(entry.name, entry.value);
    }
    return result;
}

OperationResult AppModel::operationResult() const {
    return VectorExpression::evaluate(m_operationExpression, namedVectors());
}

QList<GraphVector> AppModel::graphVectors(const ThemeColors& colors) const {
    QList<GraphVector> result;
    QSet<int> consumedIds;
    QList<GraphVector> chainSegments;

    if (m_arrowMode == ArrowMode::HeadToTail) {
        const QList<AdditiveTerm> terms =
            VectorExpression::additiveTerms(m_operationExpression, namedVectors());
        if (terms.size() >= 2) {
            Vector3 head;
            for (const AdditiveTerm& term : terms) {
                const VectorEntry* entry = nullptr;
                if (!term.plainName.isEmpty()) {
                    for (const VectorEntry& candidate : m_vectors) {
                        if (candidate.name == term.plainName) {
                            entry = &candidate;
                            break;
                        }
                    }
                }

                const Vector3 segmentValue = term.value * term.sign;
                if (entry != nullptr) {
                    consumedIds.insert(entry->id);
                }

                if ((entry == nullptr || entry->visible) && !segmentValue.isZero()) {
                    GraphVector graphVector;
                    graphVector.tail = head;
                    graphVector.value = segmentValue;

                    if (entry != nullptr) {
                        graphVector.id = entry->id;
                        graphVector.name = entry->name;
                        graphVector.selected = entry->id == m_selectedId;
                        if (!colors.vectorColors.isEmpty()) {
                            graphVector.color = colors.vectorColors.at(
                                entry->colorIndex % colors.vectorColors.size());
                        }
                    } else {
                        graphVector.name =
                            (term.sign < 0.0 ? QStringLiteral("\u2212") : QString()) + term.text;
                        graphVector.color = colors.result;
                    }

                    chainSegments.append(graphVector);
                }

                head += segmentValue;
            }
        }
    }

    for (const VectorEntry& entry : m_vectors) {
        if (!entry.visible || entry.value.isZero() || consumedIds.contains(entry.id)) {
            continue;
        }

        GraphVector graphVector;
        graphVector.id = entry.id;
        graphVector.name = entry.name;
        graphVector.value = entry.value;
        graphVector.selected = entry.id == m_selectedId;
        if (!colors.vectorColors.isEmpty()) {
            graphVector.color = colors.vectorColors.at(entry.colorIndex % colors.vectorColors.size());
        }
        result.append(graphVector);
    }

    result.append(chainSegments);

    const OperationResult operation = operationResult();
    if (operation.valid && operation.value.type == ExpressionValueType::Vector
        && !operation.value.vector.isZero()) {
        GraphVector graphVector;
        graphVector.id = -1;
        graphVector.name = m_operationExpression.trimmed();
        graphVector.value = operation.value.vector;
        graphVector.color = colors.result;
        graphVector.isResult = true;
        result.append(graphVector);
    }

    return result;
}

int AppModel::nextColorIndex() {
    const int index = m_nextColorIndex;
    ++m_nextColorIndex;
    return index;
}

QString AppModel::uniqueName(const QString& base) const {
    const QString prefix = base.isEmpty() ? QStringLiteral("V") : base;
    if (!hasName(prefix)) {
        return prefix;
    }

    for (int index = 1;; ++index) {
        const QString candidate = prefix + QString::number(index);
        if (!hasName(candidate)) {
            return candidate;
        }
    }
}

int AppModel::addVector(const QString& requestedName, const Vector3& value) {
    const QString finalName = uniqueName(requestedName);

    VectorEntry entry;
    entry.id = m_nextId++;
    entry.name = finalName;
    entry.value = value;
    entry.colorIndex = nextColorIndex();
    m_vectors.append(entry);

    emit vectorsChanged();
    selectVector(entry.id);
    emit operationChanged();
    return entry.id;
}

void AppModel::removeVector(int id) {
    int index = -1;
    for (int i = 0; i < m_vectors.size(); ++i) {
        if (m_vectors.at(i).id == id) {
            index = i;
            break;
        }
    }
    if (index < 0) {
        return;
    }

    const bool wasSelected = m_selectedId == id;
    m_vectors.removeAt(index);
    emit vectorsChanged();

    if (wasSelected) {
        if (m_vectors.isEmpty()) {
            m_selectedId = -1;
            emit selectionChanged(m_selectedId);
        } else {
            selectVector(m_vectors.first().id);
        }
    }

    emit operationChanged();
}

bool AppModel::renameVector(int id, const QString& name, QString* error) {
    VectorEntry* entry = vectorByIdMutable(id);
    if (entry == nullptr) {
        if (error != nullptr) {
            *error = QStringLiteral("Vector not found.");
        }
        return false;
    }

    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty()) {
        if (error != nullptr) {
            *error = QStringLiteral("Enter a name.");
        }
        return false;
    }

    if (trimmed == entry->name) {
        return true;
    }

    if (!isValidName(trimmed, error)) {
        return false;
    }

    if (hasName(trimmed, id)) {
        if (error != nullptr) {
            *error = QStringLiteral("A vector named '%1' already exists.").arg(trimmed);
        }
        return false;
    }

    const QString oldName = entry->name;
    entry->name = trimmed;

    if (!oldName.isEmpty()) {
        const QRegularExpression pattern(
            QStringLiteral("\\b%1\\b").arg(QRegularExpression::escape(oldName)));
        m_operationExpression.replace(pattern, trimmed);
    }

    emit vectorUpdated(id);
    emit operationChanged();
    return true;
}

void AppModel::updateVectorValue(int id, const Vector3& value) {
    VectorEntry* entry = vectorByIdMutable(id);
    if (entry == nullptr || entry->value == value) {
        return;
    }

    entry->value = value;
    emit vectorUpdated(id);
    emit operationChanged();
}

void AppModel::setVisible(int id, bool visible) {
    VectorEntry* entry = vectorByIdMutable(id);
    if (entry == nullptr || entry->visible == visible) {
        return;
    }

    entry->visible = visible;
    emit vectorUpdated(id);
}

void AppModel::selectVector(int id) {
    if (id != -1 && vectorById(id) == nullptr) {
        return;
    }
    if (m_selectedId == id) {
        return;
    }

    m_selectedId = id;
    emit selectionChanged(m_selectedId);
}

void AppModel::setInputFormat(InputFormat format) {
    if (m_inputFormat == format) {
        return;
    }
    m_inputFormat = format;
    emit settingsChanged();
}

void AppModel::setViewMode(ViewMode mode) {
    if (m_viewMode == mode) {
        return;
    }
    m_viewMode = mode;
    emit settingsChanged();
}

void AppModel::setPlane2D(Plane2D plane) {
    if (m_plane2D == plane) {
        return;
    }
    m_plane2D = plane;
    emit settingsChanged();
}

void AppModel::setThemeMode(ThemeMode mode) {
    if (m_themeMode == mode) {
        return;
    }
    m_themeMode = mode;
    emit settingsChanged();
}

void AppModel::setShowAxisNumbers(bool show) {
    if (m_showAxisNumbers == show) {
        return;
    }
    m_showAxisNumbers = show;
    emit settingsChanged();
}

void AppModel::setShowLabels(bool show) {
    if (m_showLabels == show) {
        return;
    }
    m_showLabels = show;
    emit settingsChanged();
}

void AppModel::setArrowMode(ArrowMode mode) {
    if (m_arrowMode == mode) {
        return;
    }
    m_arrowMode = mode;
    emit settingsChanged();
}

void AppModel::setOperationExpression(const QString& expression) {
    if (m_operationExpression == expression) {
        return;
    }
    m_operationExpression = expression;
    emit operationChanged();
}

void AppModel::saveOperationResult() {
    const OperationResult operation = operationResult();
    if (!operation.valid || operation.value.type != ExpressionValueType::Vector) {
        return;
    }

    QString name;
    for (int index = 1;; ++index) {
        const QString candidate = QStringLiteral("R%1").arg(index);
        if (!hasName(candidate)) {
            name = candidate;
            break;
        }
    }

    addVector(name, operation.value.vector);
}
