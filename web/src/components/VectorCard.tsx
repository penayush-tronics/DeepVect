import { useEffect, useRef, useState, type Dispatch } from 'react';
import {
  formatNumber,
  formatUnitFraction,
} from '../core/VectorExpression';
import {
  validateVectorName,
  type AppAction,
} from '../core/model';
import type { AppState, VectorEntry } from '../core/types';
import type { ThemeColors } from '../core/theme';
import VectorEditor from './VectorEditor';

interface Props {
  entry: VectorEntry;
  state: AppState;
  colors: ThemeColors;
  dispatch: Dispatch<AppAction>;
}

function angleText(component: number, length: number): string {
  if (length < 1e-12) return '\u2014';
  const angle = (Math.acos(Math.max(-1, Math.min(1, component / length))) * 180) / Math.PI;
  return `${angle.toFixed(1)}\u00B0`;
}

export default function VectorCard({ entry, state, colors, dispatch }: Props) {
  const selected = entry.id === state.selectedId;
  const [name, setName] = useState(entry.name);
  const [nameError, setNameError] = useState('');
  const nameFocused = useRef(false);
  const skipCommit = useRef(false);

  useEffect(() => {
    if (!nameFocused.current) setName(entry.name);
  }, [entry.name]);

  const commitName = () => {
    const error = validateVectorName(state, name, entry.id);
    if (error) {
      setNameError(error);
      setName(entry.name);
      return;
    }
    setNameError('');
    setName(name.trim());
    dispatch({ type: 'rename', id: entry.id, name });
  };

  const color =
    colors.vectorColors[entry.colorIndex % colors.vectorColors.length] ?? colors.accent;
  const length = entry.value.length();
  const zero = entry.value.isZero();
  const column = state.inputFormat === 'column';

  return (
    <article
      className={`vector-card${selected ? ' selected' : ''}${entry.visible ? '' : ' hidden-vector'}`}
      onClick={() => dispatch({ type: 'select', id: entry.id })}
    >
      <header className="card-header">
        <input
          className="name-input"
          value={name}
          aria-label={`Name for vector ${entry.name}`}
          onFocus={() => {
            nameFocused.current = true;
            dispatch({ type: 'select', id: entry.id });
          }}
          onBlur={() => {
            nameFocused.current = false;
            if (skipCommit.current) {
              skipCommit.current = false;
              setName(entry.name);
              setNameError('');
              return;
            }
            commitName();
          }}
          onChange={(event) => setName(event.target.value)}
          onKeyDown={(event) => {
            if (event.key === 'Enter') event.currentTarget.blur();
            if (event.key === 'Escape') {
              skipCommit.current = true;
              setName(entry.name);
              setNameError('');
              event.currentTarget.blur();
            }
          }}
        />
        <span className="color-dot" style={{ background: color }} aria-hidden="true" />
        <label className="visibility" title="Show in graph">
          <input
            type="checkbox"
            checked={entry.visible}
            aria-label={`Show vector ${entry.name}`}
            onChange={(event) => {
              dispatch({ type: 'select', id: entry.id });
              dispatch({ type: 'visible', id: entry.id, visible: event.target.checked });
            }}
          />
          <span className="checkmark" />
        </label>
        <button
          type="button"
          className="icon-button delete"
          aria-label={`Delete vector ${entry.name}`}
          title="Delete vector"
          onClick={(event) => {
            event.stopPropagation();
            dispatch({ type: 'remove', id: entry.id });
          }}
        >
          {'\u00D7'}
        </button>
      </header>

      {nameError && <div className="field-error name-error">{nameError}</div>}

      <VectorEditor
        value={entry.value}
        format={state.inputFormat}
        onChange={(value) => {
          dispatch({ type: 'select', id: entry.id });
          dispatch({ type: 'update', id: entry.id, value });
        }}
      />

      {selected && (
        <div className="card-stats">
          <div className="stat-row">
            <span>Magnitude</span>
            <strong>{formatNumber(length)}</strong>
          </div>
          <div className="stat-row">
            <span>Unit vector</span>
            <strong className="wrap-value">
              {zero ? '\u2014' : formatUnitFraction(entry.value, column)}
            </strong>
          </div>
          <div className="stat-row">
            <span>Direction</span>
            <strong className="direction">
              {`\u03B1 = ${angleText(entry.value.x, length)}, `}
              {`\u03B2 = ${angleText(entry.value.y, length)}, `}
              {`\u03B3 = ${angleText(entry.value.z, length)}`}
            </strong>
          </div>
        </div>
      )}
    </article>
  );
}
