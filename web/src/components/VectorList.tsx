import { useEffect, type Dispatch } from 'react';
import type { AppAction } from '../core/model';
import type { AppState } from '../core/types';
import type { ThemeColors } from '../core/theme';
import VectorCard from './VectorCard';

interface Props {
  state: AppState;
  colors: ThemeColors;
  dispatch: Dispatch<AppAction>;
}

export default function VectorList({ state, colors, dispatch }: Props) {
  useEffect(() => {
    const card = document.getElementById(`vector-card-${state.selectedId}`);
    card?.scrollIntoView({ block: 'nearest' });
  }, [state.selectedId]);

  return (
    <section className="vector-panel">
      <div className="panel-heading">
        <span>VECTORS</span>
        <button
          type="button"
          className="add-button"
          onClick={() => dispatch({ type: 'add' })}
          title="Add vector"
        >
          + Add
        </button>
      </div>
      <div className="vector-list">
        {state.vectors.map((entry) => (
          <div id={`vector-card-${entry.id}`} key={entry.id}>
            <VectorCard
              entry={entry}
              state={state}
              colors={colors}
              dispatch={dispatch}
            />
          </div>
        ))}
        {state.vectors.length === 0 && (
          <p className="empty-state">No vectors yet. Add one to begin.</p>
        )}
      </div>
    </section>
  );
}
