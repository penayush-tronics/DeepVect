import { useMemo, useReducer, useState, type CSSProperties } from 'react';
import Graph2D from './components/Graph2D';
import Graph3D from './components/Graph3D';
import OperationCard from './components/OperationCard';
import Toolbar from './components/Toolbar';
import VectorList from './components/VectorList';
import { appReducer, buildGraphVectors, initialState } from './core/model';
import { cssVariables, themeColors } from './core/theme';

export default function App() {
  const [state, dispatch] = useReducer(appReducer, initialState);
  const [fitSignal, setFitSignal] = useState(0);
  const colors = useMemo(() => themeColors(state.themeMode), [state.themeMode]);
  const graphVectors = useMemo(() => buildGraphVectors(state, colors), [colors, state]);
  const style = cssVariables(colors) as unknown as CSSProperties;

  return (
    <div className="app" style={style} data-theme={state.themeMode}>
      <Toolbar
        state={state}
        dispatch={dispatch}
        onFit={() => setFitSignal((value) => value + 1)}
      />

      <main className="workspace">
        <aside className="sidebar">
          <VectorList state={state} colors={colors} dispatch={dispatch} />
          <OperationCard state={state} dispatch={dispatch} />
        </aside>

        <section className="graph-panel" aria-label="Vector graph">
          <div className="graph-heading">
            <span>{state.viewMode === '3d' ? '3D VIEW' : `2D VIEW \u00B7 ${state.plane2D.toUpperCase()}`}</span>
            <span className="graph-count">
              {graphVectors.filter((vector) => !vector.isResult).length} vector
              {graphVectors.filter((vector) => !vector.isResult).length === 1 ? '' : 's'}
            </span>
          </div>
          {state.viewMode === '3d' ? (
            <Graph3D
              vectors={graphVectors}
              colors={colors}
              showAxisNumbers={state.showAxisNumbers}
              showLabels={state.showLabels}
              fitSignal={fitSignal}
            />
          ) : (
            <Graph2D
              vectors={graphVectors}
              plane={state.plane2D}
              colors={colors}
              showAxisNumbers={state.showAxisNumbers}
              showLabels={state.showLabels}
              fitSignal={fitSignal}
            />
          )}
        </section>
      </main>
    </div>
  );
}
