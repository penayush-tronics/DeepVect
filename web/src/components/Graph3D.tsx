import { useEffect, useRef } from 'react';
import * as THREE from 'three';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';
import { LineSegments2 } from 'three/addons/lines/LineSegments2.js';
import { LineSegmentsGeometry } from 'three/addons/lines/LineSegmentsGeometry.js';
import { LineMaterial } from 'three/addons/lines/LineMaterial.js';
import { CSS2DObject, CSS2DRenderer } from 'three/addons/renderers/CSS2DRenderer.js';
import { formatTick, niceStep } from '../core/VectorExpression';
import type { GraphVector } from '../core/types';
import type { ThemeColors } from '../core/theme';

interface Props {
  vectors: GraphVector[];
  colors: ThemeColors;
  showAxisNumbers: boolean;
  showLabels: boolean;
  fitSignal: number;
}

interface SceneHandles {
  scene: THREE.Scene;
  camera: THREE.PerspectiveCamera;
  renderer: THREE.WebGLRenderer;
  labelRenderer: CSS2DRenderer;
  controls: OrbitControls;
  content: THREE.Group;
  resize: () => void;
}

function disposeObject(object: THREE.Object3D): void {
  object.traverse((child) => {
    if (child instanceof CSS2DObject) {
      child.element.remove();
    }
    const mesh = child as THREE.Mesh;
    if (mesh.geometry) mesh.geometry.dispose();
    const material = mesh.material as THREE.Material | THREE.Material[] | undefined;
    if (Array.isArray(material)) {
      material.forEach((item) => item.dispose());
    } else if (material) {
      material.dispose();
    }
  });
}

function clearGroup(group: THREE.Group): void {
  for (const child of [...group.children]) {
    disposeObject(child);
    group.remove(child);
  }
}

function axisLabel(text: string, className: string, color?: string): CSS2DObject {
  const element = document.createElement('div');
  element.className = className;
  element.textContent = text;
  if (color) element.style.color = color;
  return new CSS2DObject(element);
}

function maxCoordinate(vectors: GraphVector[]): number {
  let maximum = 0;
  for (const item of vectors) {
    for (const point of [item.tail, item.tail.add(item.value)]) {
      maximum = Math.max(maximum, Math.abs(point.x), Math.abs(point.y), Math.abs(point.z));
    }
  }
  return maximum;
}

export default function Graph3D({
  vectors,
  colors,
  showAxisNumbers,
  showLabels,
  fitSignal,
}: Props) {
  const containerRef = useRef<HTMLDivElement>(null);
  const handles = useRef<SceneHandles | null>(null);
  const vectorsRef = useRef(vectors);
  const fitRef = useRef<() => void>(() => undefined);
  const initialized = useRef(false);

  vectorsRef.current = vectors;

  useEffect(() => {
    const container = containerRef.current;
    if (!container) return;

    const scene = new THREE.Scene();
    scene.background = new THREE.Color(colors.canvas);

    const camera = new THREE.PerspectiveCamera(45, 1, 0.01, 5000);
    camera.up.set(0, 0, 1);
    camera.position.set(7, -9, 7);

    const renderer = new THREE.WebGLRenderer({ antialias: true });
    renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, 2));
    renderer.domElement.className = 'graph-canvas';
    renderer.domElement.setAttribute('aria-label', 'Interactive 3D vector graph');
    container.prepend(renderer.domElement);

    const labelRenderer = new CSS2DRenderer();
    labelRenderer.domElement.className = 'label-layer';
    container.appendChild(labelRenderer.domElement);

    const controls = new OrbitControls(camera, renderer.domElement);
    controls.enableDamping = true;
    controls.dampingFactor = 0.08;
    controls.screenSpacePanning = true;
    controls.minDistance = 0.5;
    controls.maxDistance = 4000;

    const content = new THREE.Group();
    scene.add(content);

    const resize = () => {
      const width = Math.max(1, container.clientWidth);
      const height = Math.max(1, container.clientHeight);
      camera.aspect = width / height;
      camera.updateProjectionMatrix();
      renderer.setSize(width, height, false);
      labelRenderer.setSize(width, height);
      const lineMaterials = content.children
        .map((child) => (child as LineSegments2).material)
        .filter((material): material is LineMaterial => material instanceof LineMaterial);
      lineMaterials.forEach((material) => material.resolution.set(width, height));
    };

    const observer = new ResizeObserver(resize);
    observer.observe(container);
    resize();

    let frame = 0;
    const animate = () => {
      controls.update();
      renderer.render(scene, camera);
      labelRenderer.render(scene, camera);
      frame = requestAnimationFrame(animate);
    };
    animate();

    const fit = () => {
      const current = vectorsRef.current;
      const box = new THREE.Box3();
      box.expandByPoint(new THREE.Vector3(0, 0, 0));
      for (const item of current) {
        box.expandByPoint(
          new THREE.Vector3(item.tail.x, item.tail.y, item.tail.z),
        );
        const end = item.tail.add(item.value);
        box.expandByPoint(new THREE.Vector3(end.x, end.y, end.z));
      }
      const center = box.getCenter(new THREE.Vector3());
      const size = box.getSize(new THREE.Vector3());
      const radius = Math.max(1.2, size.length() / 2);
      const distance =
        (radius / Math.sin((camera.fov * Math.PI) / 360)) * 1.22;
      const direction = new THREE.Vector3(1, -1.15, 0.95).normalize();
      camera.position.copy(center).addScaledVector(direction, distance);
      camera.near = Math.max(0.01, distance / 2000);
      camera.far = Math.max(100, distance * 20);
      camera.updateProjectionMatrix();
      controls.target.copy(center);
      controls.update();
    };

    handles.current = { scene, camera, renderer, labelRenderer, controls, content, resize };
    fitRef.current = fit;
    initialized.current = true;
    fit();
    resize();

    return () => {
      initialized.current = false;
      cancelAnimationFrame(frame);
      observer.disconnect();
      controls.dispose();
      clearGroup(content);
      renderer.dispose();
      renderer.domElement.remove();
      labelRenderer.domElement.remove();
      handles.current = null;
    };
    // The scene is created once; colors and content are updated by separate effects.
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  useEffect(() => {
    const scene = handles.current?.scene;
    if (scene) scene.background = new THREE.Color(colors.canvas);
  }, [colors.canvas]);

  useEffect(() => {
    if (initialized.current && fitSignal >= 0) {
      fitRef.current();
    }
  }, [fitSignal]);

  useEffect(() => {
    const current = handles.current;
    if (!current) return;
    const { content, renderer } = current;
    clearGroup(content);

    const extent = Math.max(5, Math.ceil(maxCoordinate(vectors) * 1.35));
    const rendererSize = renderer.getSize(new THREE.Vector2());

    const grid = new THREE.GridHelper(
      extent * 2,
      20,
      new THREE.Color(colors.gridMajor),
      new THREE.Color(colors.gridMinor),
    );
    grid.rotation.x = Math.PI / 2;
    grid.position.z = -extent * 0.001;
    content.add(grid);

    const axes: Array<[THREE.Vector3, THREE.Vector3, string]> = [
      [new THREE.Vector3(-extent, 0, 0), new THREE.Vector3(extent, 0, 0), colors.axisX],
      [new THREE.Vector3(0, -extent, 0), new THREE.Vector3(0, extent, 0), colors.axisY],
      [new THREE.Vector3(0, 0, -extent), new THREE.Vector3(0, 0, extent), colors.axisZ],
    ];

    for (const [start, end, color] of axes) {
      const geometry = new LineSegmentsGeometry().setPositions([
        start.x,
        start.y,
        start.z,
        end.x,
        end.y,
        end.z,
      ]);
      const material = new LineMaterial({
        color: new THREE.Color(color),
        linewidth: 1.3,
        transparent: true,
        opacity: 0.9,
      });
      material.resolution.copy(rendererSize);
      const line = new LineSegments2(geometry, material);
      content.add(line);
    }

    const axisNames: Array<[string, string, THREE.Vector3]> = [
      ['x', colors.axisX, new THREE.Vector3(extent * 0.5, 0, 0)],
      ['y', colors.axisY, new THREE.Vector3(0, extent * 0.5, 0)],
      ['z', colors.axisZ, new THREE.Vector3(0, 0, extent * 0.5)],
    ];
    for (const [name, color, position] of axisNames) {
      const label = axisLabel(name, 'axis-label', color);
      label.position.copy(position);
      content.add(label);
    }

    if (showAxisNumbers) {
      const step = niceStep(extent / 4);
      for (let value = -extent; value <= extent + step * 0.01; value += step) {
        if (Math.abs(value) < step * 1e-6) continue;
        const text = formatTick(value, step);
        const xLabel = axisLabel(text, 'axis-number');
        xLabel.position.set(value, 0, -extent * 0.012);
        content.add(xLabel);
        const yLabel = axisLabel(text, 'axis-number');
        yLabel.position.set(0, value, -extent * 0.012);
        content.add(yLabel);
        if (extent > 3) {
          const zLabel = axisLabel(text, 'axis-number');
          zLabel.position.set(-extent * 0.012, 0, value);
          content.add(zLabel);
        }
      }
    }

    const ordered = [...vectors].sort((a, b) => {
      if (a.isResult !== b.isResult) return a.isResult ? 1 : -1;
      if (a.selected !== b.selected) return a.selected ? 1 : -1;
      return 0;
    });

    for (const item of ordered) {
      const tail = new THREE.Vector3(item.tail.x, item.tail.y, item.tail.z);
      const end = new THREE.Vector3(
        item.tail.x + item.value.x,
        item.tail.y + item.value.y,
        item.tail.z + item.value.z,
      );
      const direction = end.clone().sub(tail);
      const length = direction.length();
      if (length < 1e-9) continue;
      direction.normalize();

      const headLength = Math.min(
        Math.max(0.16, extent * 0.032),
        length * 0.32,
      );
      const base = end.clone().addScaledVector(direction, -headLength);
      const color = new THREE.Color(item.color);
      const positions: number[] = [];

      if (item.isResult) {
        const dashedLength = tail.distanceTo(base);
        const dash = Math.max(extent * 0.012, dashedLength * 0.04);
        const gap = dash * 0.72;
        for (let offset = 0; offset < dashedLength; offset += dash + gap) {
          const dashEnd = Math.min(dashedLength, offset + dash);
          const from = tail.clone().addScaledVector(direction, offset);
          const to = tail.clone().addScaledVector(direction, dashEnd);
          positions.push(from.x, from.y, from.z, to.x, to.y, to.z);
        }
      } else {
        positions.push(tail.x, tail.y, tail.z, base.x, base.y, base.z);
      }

      if (positions.length > 0) {
        const geometry = new LineSegmentsGeometry().setPositions(positions);
        const material = new LineMaterial({
          color,
          linewidth: item.selected ? 4.8 : item.isResult ? 3.4 : 3.2,
        });
        material.resolution.copy(rendererSize);
        content.add(new LineSegments2(geometry, material));
      }

      const coneGeometry = new THREE.ConeGeometry(headLength * 0.4, headLength, 16);
      const coneMaterial = new THREE.MeshBasicMaterial({ color });
      const cone = new THREE.Mesh(coneGeometry, coneMaterial);
      cone.position.copy(end).addScaledVector(direction, -headLength / 2);
      cone.quaternion.setFromUnitVectors(
        new THREE.Vector3(0, 1, 0),
        direction,
      );
      content.add(cone);

      if (showLabels) {
        const label = axisLabel(item.name, 'vector-label', item.color);
        label.position
          .copy(end)
          .addScaledVector(direction, headLength * 1.15);
        if (item.isResult) label.position.z += extent * 0.045;
        content.add(label);
      }
    }

    current.resize();
  }, [colors, showAxisNumbers, showLabels, vectors]);

  useEffect(() => {
    return () => {
      initialized.current = false;
    };
  }, []);

  return <div className="graph-container graph3d" ref={containerRef} />;
}
