import type { ThemeMode } from './types';

export interface ThemeColors {
  appBackground: string;
  panel: string;
  canvas: string;
  border: string;
  text: string;
  mutedText: string;
  accent: string;
  accentSoft: string;
  gridMinor: string;
  gridMajor: string;
  axis: string;
  axisX: string;
  axisY: string;
  axisZ: string;
  selection: string;
  error: string;
  result: string;
  vectorColors: string[];
}

export const themes: Record<ThemeMode, ThemeColors> = {
  light: {
    appBackground: '#f6f7f9',
    panel: '#ffffff',
    canvas: '#ffffff',
    border: '#e4e7ec',
    text: '#1f2328',
    mutedText: '#6b7280',
    accent: '#2d70b3',
    accentSoft: '#e8f0f9',
    gridMinor: '#eceff3',
    gridMajor: '#dadfe6',
    axis: '#2b2f34',
    axisX: '#cb3f3a',
    axisY: '#227e53',
    axisZ: '#1d5fa8',
    selection: '#d9e8f8',
    error: '#c0382b',
    result: '#634099',
    vectorColors: [
      '#1f63b0',
      '#0e857b',
      '#c23b2e',
      '#6b46a8',
      '#b26a00',
      '#2a7d4f',
      '#a63a66',
      '#44515f',
    ],
  },
  dark: {
    appBackground: '#151619',
    panel: '#1e1f24',
    canvas: '#1b1c20',
    border: '#2e3138',
    text: '#e8eaed',
    mutedText: '#9aa0a6',
    accent: '#6fa8dc',
    accentSoft: '#263544',
    gridMinor: '#27292f',
    gridMajor: '#363942',
    axis: '#c9ced7',
    axisX: '#f07a7a',
    axisY: '#79dda5',
    axisZ: '#80b8eb',
    selection: '#253240',
    error: '#f07a7a',
    result: '#d2bbf5',
    vectorColors: [
      '#74b9f0',
      '#5fe0d1',
      '#ff9a8f',
      '#c8b3f4',
      '#ffc168',
      '#7de0a9',
      '#f29cc7',
      '#c8ced6',
    ],
  },
};

export function themeColors(mode: ThemeMode): ThemeColors {
  return themes[mode];
}

export function cssVariables(colors: ThemeColors): Record<string, string> {
  return {
    '--app-bg': colors.appBackground,
    '--panel': colors.panel,
    '--canvas': colors.canvas,
    '--border': colors.border,
    '--text': colors.text,
    '--muted': colors.mutedText,
    '--accent': colors.accent,
    '--accent-soft': colors.accentSoft,
    '--selection': colors.selection,
    '--error': colors.error,
    '--result': colors.result,
  };
}
