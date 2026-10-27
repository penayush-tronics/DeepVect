export const EPSILON = 1e-12;

export class Vector3 {
  x: number;
  y: number;
  z: number;

  constructor(x = 0, y = 0, z = 0) {
    this.x = x;
    this.y = y;
    this.z = z;
  }

  static zero(): Vector3 {
    return new Vector3();
  }

  clone(): Vector3 {
    return new Vector3(this.x, this.y, this.z);
  }

  add(rhs: Vector3): Vector3 {
    return new Vector3(this.x + rhs.x, this.y + rhs.y, this.z + rhs.z);
  }

  sub(rhs: Vector3): Vector3 {
    return new Vector3(this.x - rhs.x, this.y - rhs.y, this.z - rhs.z);
  }

  mul(scalar: number): Vector3 {
    return new Vector3(this.x * scalar, this.y * scalar, this.z * scalar);
  }

  div(scalar: number): Vector3 {
    return this.mul(1 / scalar);
  }

  neg(): Vector3 {
    return new Vector3(-this.x, -this.y, -this.z);
  }

  dot(rhs: Vector3): number {
    return this.x * rhs.x + this.y * rhs.y + this.z * rhs.z;
  }

  cross(rhs: Vector3): Vector3 {
    return new Vector3(
      this.y * rhs.z - this.z * rhs.y,
      this.z * rhs.x - this.x * rhs.z,
      this.x * rhs.y - this.y * rhs.x,
    );
  }

  lengthSquared(): number {
    return this.dot(this);
  }

  length(): number {
    return Math.hypot(this.x, this.y, this.z);
  }

  normalized(): Vector3 {
    const length = this.length();
    return length < EPSILON ? new Vector3() : this.div(length);
  }

  component(index: number): number {
    if (index === 0) return this.x;
    if (index === 1) return this.y;
    return this.z;
  }

  isZero(epsilon = EPSILON): boolean {
    return Math.abs(this.x) < epsilon && Math.abs(this.y) < epsilon && Math.abs(this.z) < epsilon;
  }

  equals(other: Vector3, epsilon = EPSILON): boolean {
    return (
      Math.abs(this.x - other.x) < epsilon &&
      Math.abs(this.y - other.y) < epsilon &&
      Math.abs(this.z - other.z) < epsilon
    );
  }
}
