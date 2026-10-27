import { describe, expect, it } from 'vitest';
import {
  appReducer,
  buildGraphVectors,
  initialState,
  operationResult,
  validateVectorName,
} from './model';
import { themes } from './theme';
import { Vector3 } from './Vector3';

describe('initial state', () => {
  it('matches the desktop defaults', () => {
    expect(initialState.vectors.map((entry) => entry.name)).toEqual(['A', 'B']);
    expect(initialState.vectors[0].value).toEqual(new Vector3(3, 2, 1));
    expect(initialState.vectors[1].value).toEqual(new Vector3(-1, 4, 2));
    expect(initialState.operationExpression).toBe('A + B');
    expect(initialState.viewMode).toBe('3d');
    expect(initialState.themeMode).toBe('light');
  });

  it('evaluates the default operation', () => {
    const result = operationResult(initialState);
    expect(result.valid).toBe(true);
    expect(result.value?.type).toBe('vector');
    if (result.value?.type === 'vector') {
      expect(result.value.vector).toEqual(new Vector3(2, 6, 3));
    }
  });
});

describe('vector names', () => {
  it('rejects reserved names', () => {
    for (const name of ['i', 'j', 'k', 'x', 'X', 'dot', 'cross']) {
      expect(validateVectorName(initialState, name)).not.toBeNull();
    }
  });

  it('rejects duplicates and invalid identifiers', () => {
    expect(validateVectorName(initialState, 'A')).not.toBeNull();
    expect(validateVectorName(initialState, '2A')).not.toBeNull();
    expect(validateVectorName(initialState, 'A B')).not.toBeNull();
  });

  it('allows renaming a vector to its current name', () => {
    expect(validateVectorName(initialState, 'A', 1)).toBeNull();
  });
});

describe('reducer', () => {
  it('adds, updates, hides, and removes vectors', () => {
    let state = appReducer(initialState, { type: 'add', value: new Vector3(1, 0, 0) });
    expect(state.vectors).toHaveLength(3);
    expect(state.vectors[2].name).toBe('V');
    expect(state.selectedId).toBe(state.vectors[2].id);

    state = appReducer(state, {
      type: 'update',
      id: state.selectedId,
      value: new Vector3(0, 2, 0),
    });
    expect(state.vectors[2].value).toEqual(new Vector3(0, 2, 0));

    state = appReducer(state, { type: 'visible', id: 1, visible: false });
    expect(state.vectors[0].visible).toBe(false);

    state = appReducer(state, { type: 'remove', id: state.selectedId });
    expect(state.vectors).toHaveLength(2);
    expect(state.selectedId).toBe(1);
  });

  it('saves a result as the next available R name', () => {
    let state = appReducer(initialState, { type: 'saveResult' });
    expect(state.vectors.at(-1)?.name).toBe('R1');
    expect(state.vectors.at(-1)?.value).toEqual(new Vector3(2, 6, 3));

    state = appReducer(state, { type: 'saveResult' });
    expect(state.vectors.at(-1)?.name).toBe('R2');
  });

  it('skips existing R names', () => {
    const withR1 = appReducer(initialState, {
      type: 'add',
      name: 'R1',
      value: new Vector3(9, 9, 9),
    });
    const state = appReducer(withR1, { type: 'saveResult' });
    expect(state.vectors.at(-1)?.name).toBe('R2');
  });

  it('does not save a scalar result', () => {
    const state = appReducer(initialState, { type: 'expression', expression: 'A . B' });
    const next = appReducer(state, { type: 'saveResult' });
    expect(next.vectors).toHaveLength(2);
  });
});

describe('graph vectors', () => {
  const colors = themes.light;

  it('draws origin vectors and a dashed result', () => {
    const graph = buildGraphVectors(initialState, colors);
    expect(graph.map((vector) => vector.name)).toEqual(['A', 'B', 'A + B']);
    expect(graph[2].isResult).toBe(true);
    expect(graph[2].value).toEqual(new Vector3(2, 6, 3));
  });

  it('chains additive vectors head to tail', () => {
    const state = { ...initialState, arrowMode: 'head-to-tail' as const };
    const graph = buildGraphVectors(state, colors);
    const names = graph.map((vector) => vector.name);
    expect(names).toEqual(['A', 'B', 'A + B']);
    expect(graph[0].tail).toEqual(new Vector3());
    expect(graph[1].tail).toEqual(new Vector3(3, 2, 1));
    expect(graph[2].isResult).toBe(true);
  });

  it('advances through hidden plain terms without drawing them', () => {
    const state = appReducer(
      { ...initialState, arrowMode: 'head-to-tail' as const },
      { type: 'visible', id: 1, visible: false },
    );
    const graph = buildGraphVectors(state, colors);
    expect(graph.map((vector) => vector.name)).toEqual(['B', 'A + B']);
    expect(graph[0].tail).toEqual(new Vector3(3, 2, 1));
  });

  it('shows composite terms in the chain', () => {
    const state = {
      ...initialState,
      arrowMode: 'head-to-tail' as const,
      operationExpression: 'A + B x C',
      vectors: [
        ...initialState.vectors,
        { id: 3, name: 'C', value: new Vector3(1, 0, 0), colorIndex: 2, visible: true },
      ],
    };
    const graph = buildGraphVectors(state, colors);
    expect(graph.map((vector) => vector.name)).toEqual(['B', 'C', 'A', 'B x C', 'A + B x C']);
    expect(graph[3].value).toEqual(new Vector3(0, 2, -4));
  });
});
