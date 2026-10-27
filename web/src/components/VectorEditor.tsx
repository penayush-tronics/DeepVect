import { useEffect, useRef, useState, type FocusEvent } from 'react';
import {
  formatIJK,
  formatNumber,
  parseVector,
} from '../core/VectorExpression';
import type { InputFormat } from '../core/types';
import { Vector3 } from '../core/Vector3';

interface Props {
  value: Vector3;
  format: InputFormat;
  onChange: (value: Vector3) => void;
}

export default function VectorEditor({ value, format, onChange }: Props) {
  const [ijkText, setIjkText] = useState(() => formatIJK(value));
  const [columns, setColumns] = useState(() => [
    formatNumber(value.x),
    formatNumber(value.y),
    formatNumber(value.z),
  ]);
  const [error, setError] = useState('');
  const focused = useRef(false);
  const lastFormat = useRef(format);

  useEffect(() => {
    const formatChanged = lastFormat.current !== format;
    lastFormat.current = format;
    if (!focused.current || formatChanged) {
      setIjkText(formatIJK(value));
      setColumns([formatNumber(value.x), formatNumber(value.y), formatNumber(value.z)]);
      setError('');
    }
  }, [value, format]);

  const commitIjk = (text: string) => {
    setIjkText(text);
    const parsed = parseVector(text);
    if (parsed.value) {
      setError('');
      onChange(parsed.value);
    } else {
      setError(parsed.error);
    }
  };

  const commitColumns = (nextColumns: string[]) => {
    setColumns(nextColumns);
    const numbers = nextColumns.map((text) => Number(text.trim()));
    if (numbers.some((number) => !Number.isFinite(number))) {
      setError('Enter a valid number in each row.');
      return;
    }
    setError('');
    onChange(new Vector3(numbers[0], numbers[1], numbers[2]));
  };

  const handleBlur = (event: FocusEvent<HTMLDivElement>) => {
    if (event.currentTarget.contains(event.relatedTarget as Node | null)) return;
    focused.current = false;
    if (error) return;
    setIjkText(formatIJK(value));
    setColumns([formatNumber(value.x), formatNumber(value.y), formatNumber(value.z)]);
  };

  if (format === 'column') {
    return (
      <div
        className="vector-editor column-editor"
        onFocus={() => {
          focused.current = true;
        }}
        onBlur={handleBlur}
      >
        <span className="bracket">[</span>
        <div className="column-fields">
          {columns.map((text, index) => (
            <input
              key={['x', 'y', 'z'][index]}
              value={text}
              aria-label={`${['x', 'y', 'z'][index]} component`}
              onChange={(event) => {
                const next = [...columns];
                next[index] = event.target.value;
                commitColumns(next);
              }}
            />
          ))}
        </div>
        <span className="bracket">]</span>
        {error && <div className="field-error">{error}</div>}
      </div>
    );
  }

  return (
    <div
      className="vector-editor"
      onFocus={() => {
        focused.current = true;
      }}
      onBlur={handleBlur}
    >
      <input
        value={ijkText}
        placeholder="3i + 2j + k"
        aria-label="Vector in i j k form"
        onChange={(event) => commitIjk(event.target.value)}
      />
      {error && <div className="field-error">{error}</div>}
    </div>
  );
}
