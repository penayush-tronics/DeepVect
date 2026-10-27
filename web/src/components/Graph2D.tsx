import {
  useCallback,
  useEffect,
  useRef,
  useState,
  type PointerEvent as ReactPointerEvent,
  type WheelEvent as ReactWheelEvent,
} from 'react';
import { formatTick, niceStep } from '../core/VectorExpression';
import type { GraphVector, Plane2D } from '../core/types';
import type { ThemeColors } from '../core/theme';
import { Vector3 } from '../core/Vector3';

interface Props {
  vectors: GraphVector[];
  plane: Plane2D;
  colors: ThemeColors;
  showAxisNumbers: boolean;
  showLabels: boolean;
  fitSignal: number;
}

interface View {
  centerU: number;
  centerV: number;
  scale: number;
}

interface Size {
  width: number;
  height: number;
}

function planeComponents(plane: Plane2D): [number, number, string, string] {
  if (plane === 'xz') return [0, 2, 'x', 'z'];
  if (plane === 'yz') return [1, 2, 'y', 'z'];
  return [0, 1, 'x', 'y'];
}

function components(plane: Plane2D, value: Vector3): [number, number] {
  const [u, v] = planeComponents(plane);
  return [value.component(u), value.component(v)];
}

function clamp(value: number, minimum: number, maximum: number): number {
  return Math.max(minimum, Math.min(maximum, value));
}

function arrowPath(
  ctx: CanvasRenderingContext2D,
  fromX: number,
  fromY: number,
  toX: number,
  toY: number,
  width: number,
  dashed: boolean,
): void {
  const dx = toX - fromX;
  const dy = toY - fromY;
  const length = Math.hypot(dx, dy);
  if (length < 0.5) return;

  const ux = dx / length;
  const uy = dy / length;
  const headLength = Math.min(Math.max(11, width * 3.2), length * 0.55);
  const halfWidth = headLength * 0.42;
  const baseX = toX - ux * headLength;
  const baseY = toY - uy * headLength;

  ctx.save();
  ctx.lineCap = 'round';
  ctx.lineJoin = 'round';
  ctx.lineWidth = width;
  ctx.strokeStyle = ctx.fillStyle;
  if (dashed) ctx.setLineDash([8, 6]);
  ctx.beginPath();
  ctx.moveTo(fromX, fromY);
  ctx.lineTo(baseX, baseY);
  ctx.stroke();
  ctx.setLineDash([]);

  ctx.beginPath();
  ctx.moveTo(toX, toY);
  ctx.lineTo(baseX - uy * halfWidth, baseY + ux * halfWidth);
  ctx.lineTo(baseX + uy * halfWidth, baseY - ux * halfWidth);
  ctx.closePath();
  ctx.fill();
  ctx.restore();
}

export default function Graph2D({
  vectors,
  plane,
  colors,
  showAxisNumbers,
  showLabels,
  fitSignal,
}: Props) {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const containerRef = useRef<HTMLDivElement>(null);
  const [size, setSize] = useState<Size>({ width: 0, height: 0 });
  const [view, setView] = useState<View>({ centerU: 0, centerV: 0, scale: 42 });
  const drag = useRef<{
    pointerId: number;
    startX: number;
    startY: number;
    centerU: number;
    centerV: number;
  } | null>(null);

  const fitView = useCallback(() => {
    if (size.width < 2 || size.height < 2) return;
    let minU = 0;
    let maxU = 0;
    let minV = 0;
    let maxV = 0;
    let any = false;

    for (const item of vectors) {
      const points = [item.tail, item.tail.add(item.value)];
      for (const point of points) {
        const [u, v] = components(plane, point);
        minU = any ? Math.min(minU, u) : u;
        maxU = any ? Math.max(maxU, u) : u;
        minV = any ? Math.min(minV, v) : v;
        maxV = any ? Math.max(maxV, v) : v;
        any = true;
      }
    }

    if (!any) {
      minU = -5;
      maxU = 5;
      minV = -5;
      maxV = 5;
    }

    let spanU = maxU - minU;
    let spanV = maxV - minV;
    if (spanU < 0.5) {
      minU -= (0.5 - spanU) / 2 + 0.25;
      maxU += (0.5 - spanU) / 2 + 0.25;
      spanU = maxU - minU;
    }
    if (spanV < 0.5) {
      minV -= (0.5 - spanV) / 2 + 0.25;
      maxV += (0.5 - spanV) / 2 + 0.25;
      spanV = maxV - minV;
    }

    const availableWidth = Math.max(40, size.width - 48);
    const availableHeight = Math.max(40, size.height - 48);
    const scale = clamp(
      Math.min(availableWidth / spanU, availableHeight / spanV),
      3,
      3000,
    );
    setView({
      centerU: (minU + maxU) / 2,
      centerV: (minV + maxV) / 2,
      scale,
    });
  }, [plane, size.height, size.width, vectors]);

  const fitRef = useRef(fitView);

  useEffect(() => {
    const element = containerRef.current;
    if (!element) return;
    const observer = new ResizeObserver((entries) => {
      const rect = entries[0].contentRect;
      setSize({
        width: Math.max(1, Math.floor(rect.width)),
        height: Math.max(1, Math.floor(rect.height)),
      });
    });
    observer.observe(element);
    return () => observer.disconnect();
  }, []);

  useEffect(() => {
    fitRef.current = fitView;
  }, [fitView]);

  useEffect(() => {
    if (size.width > 1 && size.height > 1) fitRef.current();
  }, [size.height, size.width, plane]);

  useEffect(() => {
    if (fitSignal > 0) fitRef.current();
  }, [fitSignal]);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas || size.width < 1 || size.height < 1) return;
    const dpr = window.devicePixelRatio || 1;
    canvas.width = Math.floor(size.width * dpr);
    canvas.height = Math.floor(size.height * dpr);
    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    ctx.clearRect(0, 0, size.width, size.height);
    ctx.fillStyle = colors.canvas;
    ctx.fillRect(0, 0, size.width, size.height);

    const { width, height } = size;
    const { centerU, centerV, scale } = view;
    const worldLeft = centerU - width / (2 * scale);
    const worldRight = centerU + width / (2 * scale);
    const worldBottom = centerV - height / (2 * scale);
    const worldTop = centerV + height / (2 * scale);

    const toScreen = (u: number, v: number): [number, number] => [
      width / 2 + (u - centerU) * scale,
      height / 2 - (v - centerV) * scale,
    ];

    const step = niceStep(80 / scale);
    const minor = step / 5;

    if (minor * scale >= 8) {
      ctx.strokeStyle = colors.gridMinor;
      ctx.lineWidth = 1;
      ctx.beginPath();
      const firstMinorU = Math.floor(worldLeft / minor) * minor;
      for (let value = firstMinorU; value <= worldRight; value += minor) {
        if (Math.abs(value) < minor * 1e-6) continue;
        const [x] = toScreen(value, 0);
        ctx.moveTo(x, 0);
        ctx.lineTo(x, height);
      }
      const firstMinorV = Math.floor(worldBottom / minor) * minor;
      for (let value = firstMinorV; value <= worldTop; value += minor) {
        if (Math.abs(value) < minor * 1e-6) continue;
        const [, y] = toScreen(0, value);
        ctx.moveTo(0, y);
        ctx.lineTo(width, y);
      }
      ctx.stroke();
    }

    ctx.strokeStyle = colors.gridMajor;
    ctx.lineWidth = 1;
    ctx.beginPath();
    const firstU = Math.floor(worldLeft / step) * step;
    for (let value = firstU; value <= worldRight; value += step) {
      if (Math.abs(value) < step * 1e-6) continue;
      const [x] = toScreen(value, 0);
      ctx.moveTo(x, 0);
      ctx.lineTo(x, height);
    }
    const firstV = Math.floor(worldBottom / step) * step;
    for (let value = firstV; value <= worldTop; value += step) {
      if (Math.abs(value) < step * 1e-6) continue;
      const [, y] = toScreen(0, value);
      ctx.moveTo(0, y);
      ctx.lineTo(width, y);
    }
    ctx.stroke();

    const [originX, originY] = toScreen(0, 0);
    ctx.strokeStyle = colors.axis;
    ctx.lineWidth = 1.35;
    ctx.beginPath();
    ctx.moveTo(0, originY);
    ctx.lineTo(width, originY);
    ctx.moveTo(originX, 0);
    ctx.lineTo(originX, height);
    ctx.stroke();

    if (showAxisNumbers) {
      ctx.fillStyle = colors.mutedText;
      ctx.font = '11px Inter, system-ui, sans-serif';
      ctx.textAlign = 'center';
      ctx.textBaseline = 'top';
      const labelY = clamp(originY + 5, 2, height - 16);
      for (let value = firstU; value <= worldRight; value += step) {
        if (Math.abs(value) < step * 1e-6) continue;
        const [x] = toScreen(value, 0);
        if (x < 18 || x > width - 18) continue;
        ctx.fillText(formatTick(value, step), x, labelY);
      }
      ctx.textAlign = 'right';
      ctx.textBaseline = 'middle';
      const labelX = clamp(originX - 6, 18, width - 2);
      for (let value = firstV; value <= worldTop; value += step) {
        if (Math.abs(value) < step * 1e-6) continue;
        const [, y] = toScreen(0, value);
        if (y < 12 || y > height - 12) continue;
        ctx.fillText(formatTick(value, step), labelX, y);
      }
    }

    const [, , nameU, nameV] = planeComponents(plane);
    ctx.font = '600 12px Inter, system-ui, sans-serif';
    ctx.fillStyle = colors.axis;
    ctx.textAlign = 'right';
    ctx.textBaseline = 'bottom';
    ctx.fillText(nameU, width - 10, height - 8);
    ctx.textAlign = 'left';
    ctx.textBaseline = 'top';
    ctx.fillText(nameV, 10, 8);

    const ordered = [...vectors].sort((a, b) => {
      if (a.isResult !== b.isResult) return a.isResult ? 1 : -1;
      if (a.selected !== b.selected) return a.selected ? 1 : -1;
      return 0;
    });

    const placedLabels: Array<{
      left: number;
      right: number;
      top: number;
      bottom: number;
    }> = [];

    for (const item of ordered) {
      const [tu, tv] = components(plane, item.tail);
      const end = item.tail.add(item.value);
      const [hu, hv] = components(plane, end);
      const [x1, y1] = toScreen(tu, tv);
      const [x2, y2] = toScreen(hu, hv);

      ctx.fillStyle = item.color;
      ctx.strokeStyle = item.color;
      arrowPath(
        ctx,
        x1,
        y1,
        x2,
        y2,
        item.selected ? 5 : item.isResult ? 3.4 : 3.2,
        item.isResult,
      );

      if (showLabels) {
        const dx = x2 - x1;
        const dy = y2 - y1;
        const length = Math.hypot(dx, dy);
        if (length > 1) {
          const offset = 12;
          let lx = x2 + (dx / length) * offset;
          let ly = y2 + (dy / length) * offset;
          ly = clamp(ly, 14, height - 8);
          ctx.font = `${item.selected ? '700' : '600'} 12px Inter, system-ui, sans-serif`;
          const alignRight = lx > width / 2;
          const labelWidth = ctx.measureText(item.name).width;
          lx = alignRight
            ? clamp(lx, labelWidth + 6, width - 6)
            : clamp(lx, 6, width - labelWidth - 6);

          let box = {
            left: alignRight ? lx - labelWidth : lx,
            right: alignRight ? lx : lx + labelWidth,
            top: ly - 7,
            bottom: ly + 7,
          };
          for (let attempt = 0; attempt < 6; attempt += 1) {
            const overlaps = placedLabels.some(
              (placed) =>
                box.left < placed.right + 3 &&
                box.right + 3 > placed.left &&
                box.top < placed.bottom + 2 &&
                box.bottom + 2 > placed.top,
            );
            if (!overlaps) break;
            ly += 15;
            box = { ...box, top: box.top + 15, bottom: box.bottom + 15 };
          }
          ly = Math.min(ly, height - 8);
          box = {
            left: alignRight ? lx - labelWidth : lx,
            right: alignRight ? lx : lx + labelWidth,
            top: ly - 7,
            bottom: ly + 7,
          };
          placedLabels.push(box);

          ctx.textAlign = alignRight ? 'right' : 'left';
          ctx.textBaseline = 'middle';
          ctx.lineWidth = 4;
          ctx.strokeStyle = colors.canvas;
          ctx.strokeText(item.name, lx, ly);
          ctx.fillStyle = item.color;
          ctx.fillText(item.name, lx, ly);
        }
      }
    }
  }, [colors, plane, showAxisNumbers, showLabels, size, vectors, view]);

  const onPointerDown = (event: ReactPointerEvent<HTMLCanvasElement>) => {
    if (event.button !== 0) return;
    event.currentTarget.setPointerCapture(event.pointerId);
    drag.current = {
      pointerId: event.pointerId,
      startX: event.clientX,
      startY: event.clientY,
      centerU: view.centerU,
      centerV: view.centerV,
    };
  };

  const onPointerMove = (event: ReactPointerEvent<HTMLCanvasElement>) => {
    const current = drag.current;
    if (!current || current.pointerId !== event.pointerId) return;
    const dx = event.clientX - current.startX;
    const dy = event.clientY - current.startY;
    setView((previous) => ({
      ...previous,
      centerU: current.centerU - dx / previous.scale,
      centerV: current.centerV + dy / previous.scale,
    }));
  };

  const endDrag = (event: ReactPointerEvent<HTMLCanvasElement>) => {
    if (drag.current?.pointerId === event.pointerId) {
      if (event.currentTarget.hasPointerCapture(event.pointerId)) {
        event.currentTarget.releasePointerCapture(event.pointerId);
      }
      drag.current = null;
    }
  };

  const onWheel = (event: ReactWheelEvent<HTMLCanvasElement>) => {
    event.preventDefault();
    const rect = event.currentTarget.getBoundingClientRect();
    const sx = event.clientX - rect.left;
    const sy = event.clientY - rect.top;
    const beforeU = view.centerU + (sx - size.width / 2) / view.scale;
    const beforeV = view.centerV - (sy - size.height / 2) / view.scale;
    const nextScale = clamp(view.scale * Math.exp(-event.deltaY * 0.0014), 3, 4000);
    setView({
      scale: nextScale,
      centerU: beforeU - (sx - size.width / 2) / nextScale,
      centerV: beforeV + (sy - size.height / 2) / nextScale,
    });
  };

  return (
    <div className="graph-container" ref={containerRef}>
      <canvas
        ref={canvasRef}
        className="graph-canvas"
        aria-label={`2D vector graph in the ${plane.toUpperCase()} plane`}
        onPointerDown={onPointerDown}
        onPointerMove={onPointerMove}
        onPointerUp={endDrag}
        onPointerCancel={endDrag}
        onWheel={onWheel}
      />
      <div className="graph-hint">Drag to pan &middot; Scroll to zoom</div>
    </div>
  );
}
