import type { Dispatch } from 'react';
import {
  formatNumber,
  formatUnitFraction,
  formatVector,
} from '../core/VectorExpression';
import { operationResult, type AppAction } from '../core/model';
import type { AppState } from '../core/types';

interface Props {
  state: AppState;
  dispatch: Dispatch<AppAction>;
}

export default function OperationCard({ state, dispatch }: Props) {
  const expression = state.operationExpression;
  const trimmed = expression.trim();
  const result = operationResult(state);
  const column = state.inputFormat === 'column';

  return (
    <section className="operation-panel">
      <div className="panel-heading">
        <span>OPERATION</span>
      </div>
      <div className="operation-body">
        <input
          className="expression-input"
          value={expression}
          aria-label="Vector operation expression"
          placeholder="A + B, A . B, A x B"
          spellCheck={false}
          onChange={(event) =>
            dispatch({ type: 'expression', expression: event.target.value })
          }
        />
        <p className="operation-hint">
          Use <code>.</code> for dot and <code>x</code> for cross. Parentheses are
          supported.
        </p>

        {trimmed && !result.valid && result.error && (
          <div className="operation-error">{result.error}</div>
        )}

        {result.valid && result.value?.type === 'scalar' && (
          <div className="operation-result">
            <div className="result-label">DOT PRODUCT</div>
            <div className="scalar-result">{formatNumber(result.value.scalar)}</div>
          </div>
        )}

        {result.valid && result.value?.type === 'vector' && (
          <div className="operation-result">
            <div className="result-label">RESULT</div>
            <div className="result-vector">
              {formatVector(result.value.vector, column)}
            </div>
            <div className="stat-row">
              <span>Magnitude</span>
              <strong>{formatNumber(result.value.vector.length())}</strong>
            </div>
            <div className="stat-row">
              <span>Unit vector</span>
              <strong className="wrap-value">
                {result.value.vector.isZero()
                  ? '\u2014'
                  : formatUnitFraction(result.value.vector, column)}
              </strong>
            </div>
            <button
              type="button"
              className="save-button"
              onClick={() => dispatch({ type: 'saveResult' })}
            >
              Save as new vector
            </button>
          </div>
        )}
      </div>
    </section>
  );
}
