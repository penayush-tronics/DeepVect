import { Vector3 } from './Vector3';

const EPSILON = 1e-12;

export type ExpressionValue =
  | { type: 'scalar'; scalar: number }
  | { type: 'vector'; vector: Vector3 };

export interface OperationResult {
  valid: boolean;
  error: string;
  value: ExpressionValue | null;
}

export interface AdditiveTerm {
  sign: number;
  text: string;
  plainName: string;
  value: Vector3;
}

export interface ParseResult<T> {
  value: T | null;
  error: string;
}

type TokenType =
  | 'end'
  | 'identifier'
  | 'number'
  | 'plus'
  | 'minus'
  | 'dot'
  | 'cross'
  | 'lparen'
  | 'rparen'
  | 'comma'
  | 'invalid';

interface Token {
  type: TokenType;
  text: string;
  position: number;
}

function tokenize(text: string): ParseResult<Token[]> {
  const tokens: Token[] = [];
  let pos = 0;

  while (pos < text.length) {
    const ch = text[pos];
    if (/\s/.test(ch)) {
      pos += 1;
      continue;
    }

    if (/[A-Za-z_]/.test(ch)) {
      const start = pos;
      pos += 1;
      while (pos < text.length && /[A-Za-z0-9_]/.test(text[pos])) pos += 1;
      tokens.push({ type: 'identifier', text: text.slice(start, pos), position: start });
      continue;
    }

    if (/[0-9]/.test(ch)) {
      const start = pos;
      while (pos < text.length && /[0-9.]/.test(text[pos])) pos += 1;
      tokens.push({ type: 'number', text: text.slice(start, pos), position: start });
      continue;
    }

    let type: TokenType = 'invalid';
    if (ch === '+') type = 'plus';
    else if (ch === '-') type = 'minus';
    else if (ch === '.' || ch === '*' || ch === '\u00B7') type = 'dot';
    else if (ch === '\u00D7') type = 'cross';
    else if (ch === '(') type = 'lparen';
    else if (ch === ')') type = 'rparen';
    else if (ch === ',') type = 'comma';

    if (type === 'invalid') {
      return {
        value: null,
        error: `Unexpected character '${ch}' at position ${pos + 1}.`,
      };
    }

    tokens.push({ type, text: ch, position: pos });
    pos += 1;
  }

  tokens.push({ type: 'end', text: '', position: text.length });
  return { value: tokens, error: '' };
}

class OperationParser {
  private tokens: Token[];
  private vectors: Map<string, Vector3>;
  private position = 0;
  private error = '';

  constructor(tokens: Token[], vectors: Map<string, Vector3>) {
    this.tokens = tokens;
    this.vectors = vectors;
  }

  parse(): OperationResult {
    if (this.tokens.length === 1 && this.tokens[0].type === 'end') {
      return { valid: false, error: '', value: null };
    }

    const value = this.parseExpression();
    if (!value) return { valid: false, error: this.error, value: null };
    if (this.peek().type !== 'end') {
      return {
        valid: false,
        error: `Unexpected token '${this.peek().text}' at position ${this.peek().position + 1}.`,
        value: null,
      };
    }
    return { valid: true, error: '', value };
  }

  private peek(): Token {
    return this.tokens[this.position];
  }

  private advance(): Token {
    const token = this.tokens[this.position];
    if (this.position < this.tokens.length - 1) this.position += 1;
    return token;
  }

  private isCrossOperator(token: Token): boolean {
    return (
      token.type === 'cross' ||
      (token.type === 'identifier' && (token.text === 'x' || token.text === 'X'))
    );
  }

  private expect(type: TokenType, description: string): boolean {
    if (this.peek().type !== type) {
      this.error = `Expected ${description} at position ${this.peek().position + 1}.`;
      return false;
    }
    this.advance();
    return true;
  }

  private parseExpression(): ExpressionValue | null {
    let left = this.parseTerm();
    if (!left) return null;

    while (this.peek().type === 'plus' || this.peek().type === 'minus') {
      const op = this.advance();
      const right = this.parseTerm();
      if (!right) return null;
      if (left.type !== 'vector' || right.type !== 'vector') {
        this.error = 'Addition/subtraction requires vector operands.';
        return null;
      }
      left =
        op.type === 'plus'
          ? { type: 'vector', vector: left.vector.add(right.vector) }
          : { type: 'vector', vector: left.vector.sub(right.vector) };
    }

    return left;
  }

  private parseTerm(): ExpressionValue | null {
    let left = this.parsePrimary();
    if (!left) return null;

    while (this.peek().type === 'dot' || this.isCrossOperator(this.peek())) {
      const op = this.advance();
      const right = this.parsePrimary();
      if (!right) return null;

      const operation = op.type === 'dot' ? 'The dot product' : 'The cross product';
      if (left.type !== 'vector' || right.type !== 'vector') {
        this.error = `${operation} requires vector operands.`;
        return null;
      }

      if (op.type === 'dot') {
        left = { type: 'scalar', scalar: left.vector.dot(right.vector) };
      } else {
        left = { type: 'vector', vector: left.vector.cross(right.vector) };
      }
    }

    return left;
  }

  private parsePrimary(): ExpressionValue | null {
    const token = this.peek();

    if (token.type === 'identifier') {
      this.advance();

      if (token.text === 'x' || token.text === 'X') {
        this.error = `'${token.text}' is reserved for the cross product. Use A x B.`;
        return null;
      }
      if (token.text === 'dot' || token.text === 'cross') {
        this.error = "Use '.' for dot and 'x' for cross, e.g. A . B or A x B.";
        return null;
      }

      const vector = this.vectors.get(token.text);
      if (!vector) {
        this.error = `Unknown vector '${token.text}' at position ${token.position + 1}.`;
        return null;
      }
      return { type: 'vector', vector: vector.clone() };
    }

    if (token.type === 'lparen') {
      this.advance();
      const value = this.parseExpression();
      if (!value) return null;
      return this.expect('rparen', "')'") ? value : null;
    }

    if (token.type === 'number') {
      this.error = 'Numeric literals are not supported in operation equations.';
      return null;
    }

    if (token.type === 'end') {
      this.error = 'Unexpected end of expression.';
      return null;
    }

    this.error = `Expected a vector name or '(' at position ${token.position + 1}.`;
    return null;
  }
}

export function evaluate(
  expression: string,
  vectors: Map<string, Vector3>,
): OperationResult {
  const trimmed = expression.trim();
  if (!trimmed) return { valid: false, error: '', value: null };
  const tokens = tokenize(trimmed);
  if (!tokens.value) return { valid: false, error: tokens.error, value: null };
  return new OperationParser(tokens.value, vectors).parse();
}

export function additiveTerms(
  expression: string,
  vectors: Map<string, Vector3>,
): AdditiveTerm[] {
  const terms: AdditiveTerm[] = [];
  const trimmed = expression.trim();
  if (!trimmed) return terms;

  const tokenized = tokenize(trimmed);
  if (!tokenized.value) return [];

  let depth = 0;
  let start = 0;
  let sign = 1;
  let failed = false;

  const appendTerm = (endPosition: number): boolean => {
    const termText = trimmed.slice(start, endPosition).trim();
    if (!termText) return false;
    const result = evaluate(termText, vectors);
    if (!result.valid || !result.value || result.value.type !== 'vector') return false;

    const term: AdditiveTerm = {
      sign,
      text: termText,
      plainName: '',
      value: result.value.vector,
    };

    const termTokens = tokenize(termText);
    if (
      termTokens.value &&
      termTokens.value.length === 2 &&
      termTokens.value[0].type === 'identifier' &&
      !['x', 'X', 'dot', 'cross'].includes(termTokens.value[0].text)
    ) {
      term.plainName = termTokens.value[0].text;
    }

    terms.push(term);
    return true;
  };

  for (const token of tokenized.value) {
    if (token.type === 'end') break;
    if (token.type === 'lparen') depth += 1;
    else if (token.type === 'rparen') {
      depth -= 1;
      if (depth < 0) return [];
    }

    if (depth === 0 && (token.type === 'plus' || token.type === 'minus')) {
      if (!appendTerm(token.position)) {
        failed = true;
        break;
      }
      sign = token.type === 'plus' ? 1 : -1;
      start = token.position + 1;
    }
  }

  if (failed || depth !== 0 || !appendTerm(trimmed.length)) return [];
  return terms;
}

export function formatNumber(value: number): string {
  if (Math.abs(value) < EPSILON) return '0';
  return Number(value.toPrecision(6)).toString();
}

export function formatIJK(value: Vector3): string {
  const axes = [
    [value.x, 'i'],
    [value.y, 'j'],
    [value.z, 'k'],
  ] as const;

  let result = '';
  let first = true;
  for (const [component, name] of axes) {
    if (Math.abs(component) < EPSILON) continue;
    const negative = component < 0;
    const magnitude = Math.abs(component);
    if (first) {
      if (negative) result += '-';
    } else {
      result += negative ? ' - ' : ' + ';
    }
    if (Math.abs(magnitude - 1) > EPSILON) result += formatNumber(magnitude);
    result += name;
    first = false;
  }
  return first ? '0' : result;
}

export function formatVector(value: Vector3, column: boolean): string {
  if (!column) return formatIJK(value);
  return `[${formatNumber(value.x)}, ${formatNumber(value.y)}, ${formatNumber(value.z)}]`;
}

function integerSqrt(value: number): number {
  if (value <= 0) return 0;
  let root = Math.floor(Math.sqrt(value));
  while ((root + 1) * (root + 1) <= value) root += 1;
  while (root * root > value) root -= 1;
  return root;
}

function gcd(a: number, b: number): number {
  let x = Math.abs(a);
  let y = Math.abs(b);
  while (y) {
    const next = x % y;
    x = y;
    y = next;
  }
  return x;
}

function scaleToIntegers(value: Vector3): [number, number, number] | null {
  const scales = [1, 10, 100, 1000, 10000, 100000, 1000000];
  for (const scale of scales) {
    const scaled = [value.x * scale, value.y * scale, value.z * scale];
    if (scaled.some((component) => !Number.isFinite(component) || Math.abs(component) > 9e18)) {
      continue;
    }
    const rounded = scaled.map((component) => Math.round(component));
    const close = scaled.every(
      (component, index) =>
        Math.abs(component - rounded[index]) <= 1e-7 * Math.max(1, Math.abs(component)),
    );
    if (!close) continue;
    if (rounded.every((component) => component === 0)) return null;
    return [rounded[0], rounded[1], rounded[2]];
  }
  return null;
}

function fractionMagnitude(magnitude: number, denominator: number, radical: boolean): string {
  let result = String(magnitude);
  if (radical || denominator !== 1) {
    result += '/';
    if (radical) result += '\u221A';
    result += String(denominator);
  }
  return result;
}

export function formatUnitFraction(value: Vector3, column = false): string {
  const scaled = scaleToIntegers(value);
  if (!scaled) {
    const unit = value.normalized();
    return formatVector(unit, column);
  }

  let [x, y, z] = scaled;
  const denominatorSquared = x * x + y * y + z * z;
  if (denominatorSquared <= 0) return '0';

  let denominator = integerSqrt(denominatorSquared);
  let radical = denominator * denominator !== denominatorSquared;

  if (radical) {
    let bestSquare = 1;
    for (let candidate = denominator; candidate >= 2; candidate -= 1) {
      if (denominatorSquared % (candidate * candidate) !== 0) continue;
      if (x % candidate === 0 && y % candidate === 0 && z % candidate === 0) {
        bestSquare = candidate;
        break;
      }
    }
    if (bestSquare > 1) {
      x /= bestSquare;
      y /= bestSquare;
      z /= bestSquare;
      denominator = denominatorSquared / (bestSquare * bestSquare);
      radical = denominator !== 1;
    } else {
      denominator = denominatorSquared;
    }
  } else if (denominator > 0) {
    const common = gcd(gcd(x, y), gcd(z, denominator));
    if (common > 1) {
      x /= common;
      y /= common;
      z /= common;
      denominator /= common;
    }
  }

  const values = [x, y, z];
  const names = ['i', 'j', 'k'];

  if (column) {
    const parts = values.map((component) => {
      if (component === 0) return '0';
      return `${component < 0 ? '-' : ''}${fractionMagnitude(
        Math.abs(component),
        denominator,
        radical,
      )}`;
    });
    return `[${parts.join(', ')}]`;
  }

  let result = '';
  let first = true;
  for (let index = 0; index < 3; index += 1) {
    const component = values[index];
    if (component === 0) continue;
    const negative = component < 0;
    const magnitude = Math.abs(component);
    const wholeCoefficient = !radical && magnitude === denominator;

    if (first) {
      if (negative) result += '-';
    } else {
      result += negative ? ' - ' : ' + ';
    }

    if (!wholeCoefficient) {
      let coefficient = fractionMagnitude(magnitude, denominator, radical);
      if (radical || denominator !== 1) coefficient = `(${coefficient})`;
      result += coefficient;
    }
    result += names[index];
    first = false;
  }
  return first ? '0' : result;
}

function parseNumberToken(text: string, state: { position: number }): number | null {
  const start = state.position;
  let sawDigit = false;
  let sawDot = false;
  while (state.position < text.length) {
    const ch = text[state.position];
    if (/[0-9]/.test(ch)) {
      sawDigit = true;
      state.position += 1;
    } else if (ch === '.' && !sawDot) {
      sawDot = true;
      state.position += 1;
    } else {
      break;
    }
  }
  if (!sawDigit) {
    state.position = start;
    return null;
  }
  const parsed = Number(text.slice(start, state.position));
  if (!Number.isFinite(parsed)) {
    state.position = start;
    return null;
  }
  return parsed;
}

export function parseVector(input: string): ParseResult<Vector3> {
  const text = input.trim();
  if (!text) return { value: null, error: 'Enter a vector expression.' };

  if (text.startsWith('[') || text.startsWith('(')) {
    const close = text.startsWith('[') ? ']' : ')';
    if (!text.endsWith(close)) {
      return { value: null, error: `Missing closing '${close}'.` };
    }
    const parts = text.slice(1, -1).split(',');
    if (parts.length !== 3) {
      return { value: null, error: 'Column input needs exactly three numbers.' };
    }
    const numbers = parts.map((part) => Number(part.trim()));
    if (numbers.some((number) => !Number.isFinite(number))) {
      return { value: null, error: 'Each column value must be a valid number.' };
    }
    return { value: new Vector3(numbers[0], numbers[1], numbers[2]), error: '' };
  }

  const result = new Vector3();
  const state = { position: 0 };
  let termIndex = 0;

  const skipSpaces = () => {
    while (state.position < text.length && /\s/.test(text[state.position])) state.position += 1;
  };

  while (true) {
    skipSpaces();
    if (state.position >= text.length) {
      if (termIndex === 0) return { value: null, error: 'Enter a vector expression.' };
      break;
    }

    let sign = 1;
    if (text[state.position] === '+') {
      state.position += 1;
      skipSpaces();
    } else if (text[state.position] === '-') {
      sign = -1;
      state.position += 1;
      skipSpaces();
    }

    let coefficient = 1;
    let hadNumber = false;
    const parsedNumber = parseNumberToken(text, state);
    if (parsedNumber !== null) {
      coefficient = parsedNumber;
      hadNumber = true;
      skipSpaces();
    }

    if (hadNumber && text[state.position] === '*') {
      state.position += 1;
      skipSpaces();
    }

    let axis = -1;
    if (state.position < text.length) {
      if (text[state.position] === 'i') axis = 0;
      else if (text[state.position] === 'j') axis = 1;
      else if (text[state.position] === 'k') axis = 2;
      if (axis >= 0) state.position += 1;
    }

    if (axis < 0) {
      if (hadNumber && Math.abs(sign * coefficient) < EPSILON) {
        skipSpaces();
        if (
          state.position < text.length &&
          text[state.position] !== '+' &&
          text[state.position] !== '-'
        ) {
          return { value: null, error: "Expected '+' or '-' between terms." };
        }
      } else {
        return { value: null, error: 'Each term must use i, j, or k.' };
      }
    } else {
      const contribution = sign * coefficient;
      if (axis === 0) result.x += contribution;
      else if (axis === 1) result.y += contribution;
      else result.z += contribution;
    }

    termIndex += 1;
    skipSpaces();
    if (state.position >= text.length) break;
    const next = text[state.position];
    if (next === '+' || next === '-') continue;
    return { value: null, error: "Expected '+' or '-' between terms." };
  }

  return { value: result, error: '' };
}

export function niceStep(target: number): number {
  if (!(target > 0) || !Number.isFinite(target)) return 1;
  const exponent = Math.floor(Math.log10(target));
  const base = 10 ** exponent;
  const fraction = target / base;
  let nice = 10;
  if (fraction <= 1) nice = 1;
  else if (fraction <= 2) nice = 2;
  else if (fraction <= 5) nice = 5;
  return nice * base;
}

export function formatTick(value: number, step: number): string {
  if (Math.abs(value) < step * 1e-6) return '0';
  let decimals = 0;
  if (step < 1) decimals = Math.min(6, Math.ceil(-Math.log10(step)));
  return Number(value.toFixed(decimals)).toString();
}
