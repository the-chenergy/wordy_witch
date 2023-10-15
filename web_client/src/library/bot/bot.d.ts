export interface StringVector {
  size(): number;
  push_back(_0: ArrayBuffer|Uint8Array|Uint8ClampedArray|Int8Array|string): void;
  resize(_0: number, _1: ArrayBuffer|Uint8Array|Uint8ClampedArray|Int8Array|string): void;
  set(_0: number, _1: ArrayBuffer|Uint8Array|Uint8ClampedArray|Int8Array|string): boolean;
  get(_0: number): any;
  delete(): void;
}

export interface MainModule {
  StringVector: {new(): StringVector};
  loadBank(_0: StringVector, _1: number): void;
}
