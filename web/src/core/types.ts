import { Vector3 } from './Vector3';

export type InputFormat = 'ijk' | 'column';
export type ViewMode = '2d' | '3d';
export type Plane2D = 'xy' | 'xz' | 'yz';
export type ThemeMode = 'light' | 'dark';
export type ArrowMode = 'origin' | 'head-to-tail';

export interface VectorEntry {
  id: number;
  name: string;
  value: Vector3;
  colorIndex: number;
  visible: boolean;
}

export interface AppState {
  vectors: VectorEntry[];
  selectedId: number;
  nextId: number;
  nextColorIndex: number;
  inputFormat: InputFormat;
  viewMode: ViewMode;
  plane2D: Plane2D;
  themeMode: ThemeMode;
  showAxisNumbers: boolean;
  showLabels: boolean;
  arrowMode: ArrowMode;
  operationExpression: string;
}

export interface GraphVector {
  id: number;
  name: string;
  tail: Vector3;
  value: Vector3;
  color: string;
  selected: boolean;
  isResult: boolean;
}
