import { describe, expect, it } from 'vitest';
import {
  additiveTerms,
  evaluate,
  formatIJK,
  formatUnitFraction,
  parseVector,
} from './VectorExpression';
import { Vector3 } from './Vector3';

const vectors = new Map<string, Vector3>([
  ['A', new Vector3(1, 2, 3)],
  ['B', new Vector3(4, 5, 6)],
  ['C', new Vector3(1, 0, 0)],
]);

function vector(value: { x: number; y: number; z: number }): Vector3 {
  return new Vector3(value.x, value.y, value.z);
}

function expectVector(expression: string, expected: Vector3) {
  const result = evaluate(expression, vectors);
  expect(result.valid, result.error).toBe(true);
  expect(result.value?.type).toBe('vector');
  if (result.value?.type === 'vector') {
    expect(result.value.vector.x).toBeCloseTo(expected.x, 9);
    expect(result.value.vector.y).toBeCloseTo(expected.y, 9);
    expect(result.value.vector.z).toBeCloseTo(expected.z, 9);
  }
}

function expectScalar(expression: string, expected: number) {
  const result = evaluate(expression, vectors);
  expect(result.valid, result.error).toBe(true);
  expect(result.value?.type).toBe('scalar');
  if (result.value?.type === 'scalar') {
    expect(result.value.scalar).toBeCloseTo(expected, 9);
  }
}

describe('parseVector', () => {
  it.each([
    ['3i + 2j - k', new Vector3(3, 2, -1)],
    ['-2.5j + 0.5k', new Vector3(0, -2.5, 0.5)],
    ['i - j + 4k', new Vector3(1, -1, 4)],
    ['0', new Vector3()],
    ['2 * i + 3 j', new Vector3(2, 3, 0)],
    ['[1, 2, 3]', new Vector3(1, 2, 3)],
    ['(1, -2, 3.5)', new Vector3(1, -2, 3.5)],
  ])('parses %s', (input, expected) => {
    const result = parseVector(input);
    expect(result.value, result.error).not.toBeNull();
    expect(vector(result.value!)).toEqual(expected);
  });

  it.each(['', '3i +', '2', 'ai + 2j', '3i + 2j +', '[1,2]'])(
    'rejects %s',
    (input) => {
      expect(parseVector(input).value).toBeNull();
    },
  );
});

describe('formatting', () => {
  it('formats i j k vectors', () => {
    expect(formatIJK(new Vector3(3, 2, 1))).toBe('3i + 2j + k');
    expect(formatIJK(new Vector3(0, -1, 0))).toBe('-j');
    expect(formatIJK(new Vector3(-1, 0, 2.5))).toBe('-i + 2.5k');
    expect(formatIJK(new Vector3())).toBe('0');
  });

  it.each([
    [new Vector3(3, 2, 1), false, '(3/\u221A14)i + (2/\u221A14)j + (1/\u221A14)k'],
    [new Vector3(3, 4, 0), false, '(3/5)i + (4/5)j'],
    [new Vector3(6, 8, 0), false, '(3/5)i + (4/5)j'],
    [new Vector3(0.25, 0.5, 0), false, '(1/\u221A5)i + (2/\u221A5)j'],
    [new Vector3(0, -1, 0), false, '-j'],
    [new Vector3(3, 2, 1), true, '[3/\u221A14, 2/\u221A14, 1/\u221A14]'],
  ])('formats unit fractions', (input, column, expected) => {
    expect(formatUnitFraction(input, column)).toBe(expected);
  });
});

describe('evaluate', () => {
  it('adds and subtracts vectors', () => {
    expectVector('A + B', new Vector3(5, 7, 9));
    expectVector('A - B', new Vector3(-3, -3, -3));
    expectVector('A + B - C', new Vector3(4, 7, 9));
  });

  it('evaluates dot products', () => {
    expectScalar('A . B', 32);
    expectScalar('A.B', 32);
    expectScalar('A \u00B7 B', 32);
    expectScalar('A * B', 32);
  });

  it('evaluates cross products', () => {
    expectVector('A x B', new Vector3(-3, 6, -3));
    expectVector('A X B', new Vector3(-3, 6, -3));
    expectVector('A \u00D7 B', new Vector3(-3, 6, -3));
    expectVector('(A + B) x C', new Vector3(0, 9, -7));
    expectVector('A x (B + C)', new Vector3(-3, 9, -5));
  });

  it('applies cross before dot', () => {
    expectScalar('(A x B) . C', -3);
    expectScalar('A \u00D7 B \u00B7 C', -3);
  });

  it.each([
    'A . B x C',
    'A +',
    'A + 2',
    'A . B + C',
    'A \u00B7 B \u00D7 C',
    'A + D',
    'dot(A, B)',
    'A x',
    'x + A',
  ])('rejects %s', (expression) => {
    expect(evaluate(expression, vectors).valid).toBe(false);
  });

  it('treats an empty operation as neutral', () => {
    const result = evaluate('', vectors);
    expect(result.valid).toBe(false);
    expect(result.error).toBe('');
  });
});

describe('additiveTerms', () => {
  it('splits top-level additive terms', () => {
    const terms = additiveTerms('A + B x C', vectors);
    expect(terms).toHaveLength(2);
    expect(terms[0].plainName).toBe('A');
    expect(terms[1].plainName).toBe('');
    expect(terms[1].sign).toBe(1);
    expect(terms[1].value).toEqual(new Vector3(0, 6, -5));
  });

  it('handles subtracted parenthesized expressions', () => {
    const terms = additiveTerms('A - (B + C)', vectors);
    expect(terms).toHaveLength(2);
    expect(terms[1].sign).toBe(-1);
    expect(terms[1].value).toEqual(new Vector3(5, 5, 6));
  });

  it('keeps a grouped cross as one term', () => {
    expect(additiveTerms('(A + B) x C', vectors)).toHaveLength(1);
  });

  it('does not chain expressions with scalar terms', () => {
    expect(additiveTerms('A . B + C', vectors)).toEqual([]);
  });
});
