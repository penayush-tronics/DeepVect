import { useEffect, useRef, useState, type Dispatch } from 'react';
import type { AppAction } from '../core/model';
import type { AppState } from '../core/types';

interface Props {
  state: AppState;
  dispatch: Dispatch<AppAction>;
}

export default function DisplayMenu({ state, dispatch }: Props) {
  const [open, setOpen] = useState(false);
  const root = useRef<HTMLDivElement>(null);

  useEffect(() => {
    if (!open) return;
    const onPointerDown = (event: PointerEvent) => {
      if (root.current && !root.current.contains(event.target as Node)) setOpen(false);
    };
    const onKeyDown = (event: KeyboardEvent) => {
      if (event.key === 'Escape') setOpen(false);
    };
    document.addEventListener('pointerdown', onPointerDown);
    document.addEventListener('keydown', onKeyDown);
    return () => {
      document.removeEventListener('pointerdown', onPointerDown);
      document.removeEventListener('keydown', onKeyDown);
    };
  }, [open]);

  const items = [
    {
      label: 'Axis numbers',
      checked: state.showAxisNumbers,
      action: { type: 'showAxisNumbers', show: !state.showAxisNumbers } as const,
    },
    {
      label: 'Vector labels',
      checked: state.showLabels,
      action: { type: 'showLabels', show: !state.showLabels } as const,
    },
    {
      label: 'Head-to-tail',
      checked: state.arrowMode === 'head-to-tail',
      action: {
        type: 'arrowMode',
        mode: state.arrowMode === 'head-to-tail' ? 'origin' : 'head-to-tail',
      } as const,
    },
  ];

  return (
    <div className="display-menu" ref={root}>
      <button
        type="button"
        className={`tool${open ? ' active' : ''}`}
        aria-haspopup="menu"
        aria-expanded={open}
        onClick={() => setOpen((value) => !value)}
      >
        Display <span className="caret">{'\u25BE'}</span>
      </button>
      {open && (
        <div className="menu-popup" role="menu">
          {items.map((item) => (
            <button
              key={item.label}
              type="button"
              role="menuitemcheckbox"
              aria-checked={item.checked}
              className="menu-item"
              onClick={() => dispatch(item.action)}
            >
              <span className={`menu-check${item.checked ? ' checked' : ''}`}>
                {item.checked ? '\u2713' : ''}
              </span>
              {item.label}
            </button>
          ))}
        </div>
      )}
    </div>
  );
}
