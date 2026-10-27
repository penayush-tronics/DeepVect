import type { Dispatch } from 'react';
import type { AppAction } from '../core/model';
import type { AppState, Plane2D } from '../core/types';
import DisplayMenu from './DisplayMenu';

interface Props {
  state: AppState;
  dispatch: Dispatch<AppAction>;
  onFit: () => void;
}

export default function Toolbar({ state, dispatch, onFit }: Props) {
  return (
    <header className="toolbar">
      <div className="brand">
        <h1>DeepVect</h1>
        <p>Interactive vector algebra in 2D and 3D</p>
      </div>

      <div className="toolbar-controls">
        <div className="segmented" role="group" aria-label="View mode">
          <button
            type="button"
            className={state.viewMode === '2d' ? 'active' : ''}
            aria-pressed={state.viewMode === '2d'}
            onClick={() => dispatch({ type: 'viewMode', mode: '2d' })}
          >
            2D
          </button>
          <button
            type="button"
            className={state.viewMode === '3d' ? 'active' : ''}
            aria-pressed={state.viewMode === '3d'}
            onClick={() => dispatch({ type: 'viewMode', mode: '3d' })}
          >
            3D
          </button>
        </div>

        {state.viewMode === '2d' && (
          <label className="plane-select">
            <span>Plane</span>
            <select
              value={state.plane2D}
              onChange={(event) =>
                dispatch({ type: 'plane2D', plane: event.target.value as Plane2D })
              }
            >
              <option value="xy">XY</option>
              <option value="xz">XZ</option>
              <option value="yz">YZ</option>
            </select>
          </label>
        )}

        <label className="format-select">
          <span>Input</span>
          <select
            value={state.inputFormat}
            onChange={(event) =>
              dispatch({
                type: 'inputFormat',
                format: event.target.value === 'column' ? 'column' : 'ijk',
              })
            }
          >
            <option value="ijk">i j k</option>
            <option value="column">Column</option>
          </select>
        </label>

        <DisplayMenu state={state} dispatch={dispatch} />

        <button
          type="button"
          className={`tool${state.themeMode === 'dark' ? ' active' : ''}`}
          aria-pressed={state.themeMode === 'dark'}
          onClick={() =>
            dispatch({
              type: 'themeMode',
              mode: state.themeMode === 'dark' ? 'light' : 'dark',
            })
          }
        >
          Dark mode
        </button>

        <button type="button" className="tool" onClick={onFit} title="Fit graph to vectors">
          Fit
        </button>
      </div>
    </header>
  );
}
