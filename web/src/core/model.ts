import { additiveTerms, evaluate, type AdditiveTerm } from './VectorExpression';
import { Vector3 } from './Vector3';
import type {
  AppState,
  ArrowMode,
  GraphVector,
  InputFormat,
  Plane2D,
  ThemeMode,
  VectorEntry,
  ViewMode,
} from './types';
import type { ThemeColors } from './theme';

const NAME_PATTERN = /^[A-Za-z_][A-Za-z0-9_]*$/;
const RESERVED = new Set(['i', 'j', 'k', 'x', 'X', 'dot', 'cross']);

export const initialState: AppState = {
  vectors: [
    { id: 1, name: 'A', value: new Vector3(3, 2, 1), colorIndex: 0, visible: true },
    { id: 2, name: 'B', value: new Vector3(-1, 4, 2), colorIndex: 1, visible: true },
  ],
  selectedId: 1,
  nextId: 3,
  nextColorIndex: 2,
  inputFormat: 'ijk',
  viewMode: '3d',
  plane2D: 'xy',
  themeMode: 'light',
  showAxisNumbers: false,
  showLabels: true,
  arrowMode: 'origin',
  operationExpression: 'A + B',
};

export type AppAction =
  | { type: 'add'; name?: string; value?: Vector3 }
  | { type: 'remove'; id: number }
  | { type: 'rename'; id: number; name: string }
  | { type: 'update'; id: number; value: Vector3 }
  | { type: 'visible'; id: number; visible: boolean }
  | { type: 'select'; id: number }
  | { type: 'inputFormat'; format: InputFormat }
  | { type: 'viewMode'; mode: ViewMode }
  | { type: 'plane2D'; plane: Plane2D }
  | { type: 'themeMode'; mode: ThemeMode }
  | { type: 'showAxisNumbers'; show: boolean }
  | { type: 'showLabels'; show: boolean }
  | { type: 'arrowMode'; mode: ArrowMode }
  | { type: 'expression'; expression: string }
  | { type: 'saveResult' };

export function validateVectorName(
  state: AppState,
  name: string,
  exceptId = -1,
): string | null {
  const trimmed = name.trim();
  if (!trimmed) return 'Enter a name.';
  if (!NAME_PATTERN.test(trimmed)) {
    return 'Names must start with a letter or underscore and contain only letters, digits, or underscores.';
  }
  if (RESERVED.has(trimmed)) {
    return `'${trimmed}' is reserved. Choose another name.`;
  }
  if (state.vectors.some((entry) => entry.id !== exceptId && entry.name === trimmed)) {
    return `A vector named '${trimmed}' already exists.`;
  }
  return null;
}

function uniqueName(vectors: VectorEntry[], base: string): string {
  const prefix = base.trim() || 'V';
  if (!vectors.some((entry) => entry.name === prefix)) return prefix;
  for (let index = 1; ; index += 1) {
    const candidate = `${prefix}${index}`;
    if (!vectors.some((entry) => entry.name === candidate)) return candidate;
  }
}

function addVectorToState(
  state: AppState,
  requestedName: string,
  value: Vector3,
): AppState {
  const entry: VectorEntry = {
    id: state.nextId,
    name: uniqueName(state.vectors, requestedName),
    value: value.clone(),
    colorIndex: state.nextColorIndex,
    visible: true,
  };
  return {
    ...state,
    vectors: [...state.vectors, entry],
    selectedId: entry.id,
    nextId: state.nextId + 1,
    nextColorIndex: state.nextColorIndex + 1,
  };
}

export function appReducer(state: AppState, action: AppAction): AppState {
  switch (action.type) {
    case 'add':
      return addVectorToState(state, action.name ?? '', action.value ?? new Vector3(1, 0, 0));
    case 'remove': {
      const vectors = state.vectors.filter((entry) => entry.id !== action.id);
      const selectedId =
        state.selectedId === action.id ? (vectors[0]?.id ?? -1) : state.selectedId;
      return { ...state, vectors, selectedId };
    }
    case 'rename':
      return {
        ...state,
        vectors: state.vectors.map((entry) =>
          entry.id === action.id ? { ...entry, name: action.name.trim() } : entry,
        ),
      };
    case 'update':
      return {
        ...state,
        vectors: state.vectors.map((entry) =>
          entry.id === action.id ? { ...entry, value: action.value.clone() } : entry,
        ),
      };
    case 'visible':
      return {
        ...state,
        vectors: state.vectors.map((entry) =>
          entry.id === action.id ? { ...entry, visible: action.visible } : entry,
        ),
      };
    case 'select':
      return { ...state, selectedId: action.id };
    case 'inputFormat':
      return { ...state, inputFormat: action.format };
    case 'viewMode':
      return { ...state, viewMode: action.mode };
    case 'plane2D':
      return { ...state, plane2D: action.plane };
    case 'themeMode':
      return { ...state, themeMode: action.mode };
    case 'showAxisNumbers':
      return { ...state, showAxisNumbers: action.show };
    case 'showLabels':
      return { ...state, showLabels: action.show };
    case 'arrowMode':
      return { ...state, arrowMode: action.mode };
    case 'expression':
      return { ...state, operationExpression: action.expression };
    case 'saveResult': {
      const result = operationResult(state);
      if (!result.valid || result.value?.type !== 'vector') return state;
      let name = '';
      for (let index = 1; ; index += 1) {
        const candidate = `R${index}`;
        if (!state.vectors.some((entry) => entry.name === candidate)) {
          name = candidate;
          break;
        }
      }
      return addVectorToState(state, name, result.value.vector);
    }
    default:
      return state;
  }
}

export function namedVectors(state: AppState): Map<string, Vector3> {
  return new Map(state.vectors.map((entry) => [entry.name, entry.value]));
}

export function operationResult(state: AppState) {
  return evaluate(state.operationExpression, namedVectors(state));
}

export function buildGraphVectors(state: AppState, colors: ThemeColors): GraphVector[] {
  const result: GraphVector[] = [];
  const consumedIds = new Set<number>();
  const chainSegments: GraphVector[] = [];

  if (state.arrowMode === 'head-to-tail') {
    const terms = additiveTerms(state.operationExpression, namedVectors(state));
    if (terms.length >= 2) {
      let head = new Vector3();
      for (const term of terms) {
        const entry = term.plainName
          ? state.vectors.find((candidate) => candidate.name === term.plainName)
          : undefined;
        const segmentValue = term.value.mul(term.sign);
        if (entry) consumedIds.add(entry.id);

        if ((!entry || entry.visible) && !segmentValue.isZero()) {
          const color =
            entry && colors.vectorColors.length > 0
              ? colors.vectorColors[entry.colorIndex % colors.vectorColors.length]
              : colors.result;
          chainSegments.push({
            id: entry?.id ?? -1,
            name: entry ? entry.name : `${term.sign < 0 ? '\u2212' : ''}${term.text}`,
            tail: head.clone(),
            value: segmentValue,
            color,
            selected: entry?.id === state.selectedId,
            isResult: false,
          });
        }

        head = head.add(segmentValue);
      }
    }
  }

  for (const entry of state.vectors) {
    if (!entry.visible || entry.value.isZero() || consumedIds.has(entry.id)) continue;
    const color =
      colors.vectorColors.length > 0
        ? colors.vectorColors[entry.colorIndex % colors.vectorColors.length]
        : colors.accent;
    result.push({
      id: entry.id,
      name: entry.name,
      tail: new Vector3(),
      value: entry.value,
      color,
      selected: entry.id === state.selectedId,
      isResult: false,
    });
  }

  result.push(...chainSegments);

  const operation = operationResult(state);
  if (operation.valid && operation.value?.type === 'vector' && !operation.value.vector.isZero()) {
    result.push({
      id: -1,
      name: state.operationExpression.trim(),
      tail: new Vector3(),
      value: operation.value.vector,
      color: colors.result,
      selected: false,
      isResult: true,
    });
  }

  return result;
}

export function termLabel(term: AdditiveTerm): string {
  return `${term.sign < 0 ? '\u2212' : ''}${term.text}`;
}
