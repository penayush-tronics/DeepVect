# DeepVect

DeepVect is an interactive vector algebra playground available as a web app and
as a Qt desktop application. Build named vectors, write chained operations, and
explore the geometry in 2D or 3D.

**Live web app:** <https://penayush-tronics.github.io/DeepVect/>

The desktop application is named **Vectorial**.

## Features

- **Named vector list** with inline rename, per-vector color, visibility toggle,
  delete, and live statistics for the selected vector.
- **Fraction unit vectors** such as
  `(3/√14)i + (2/√14)j + (1/√14)k`, reduced instead of rounded.
- **Two input formats**: `i, j, k` expressions or `x`, `y`, `z` columns.
- **Live operation equations** with `+`, `-`, `.`/`·`/`*` (dot),
  `x`/`X`/`×` (cross), parentheses, and chained expressions.
- **Head-to-tail mode** that draws additive terms tip-to-tail while keeping the
  final result as a dashed vector from the origin.
- **Save as vector** for operation results, named `R1`, `R2`, and so on.
- **2D and 3D graphs** with adaptive grids, colored axes, labels, optional axis
  numbers, fit, pan/zoom, and orbit controls.
- **Light and dark themes** with consistent vector colors.

## Web app

The web app is built with React, TypeScript, Vite, and Three.js. It runs entirely
in the browser and has no backend.

```bash
cd web
npm install
npm run dev
```

Production build and tests:

```bash
cd web
npm test
npm run build
```

The production files are written to `web/dist`. Vite uses `/DeepVect/` as the
base path so the build works under the GitHub Pages project URL.

### Deployment

The workflow in `.github/workflows/deploy-web.yml` tests and builds the web app
on every push to `main`, then publishes `web/dist` with GitHub Pages. Enable
Pages with **Settings → Pages → Source: GitHub Actions** if it is not enabled yet.

## Desktop app

The Qt app lives at the repository root and builds as `vectorial`.

### Dependencies

- C++20 compiler
- CMake 3.20+
- Qt6 6.4+ with `Widgets`, `OpenGL`, and `OpenGLWidgets`
- Node.js 22+ only for the web app

On Ubuntu:

```bash
sudo apt install build-essential cmake qt6-base-dev libgl1-mesa-dev
```

### Build and run

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/vectorial
```

### Test

```bash
ctest --test-dir build --output-on-failure
```

The C++ suite covers vector math, parsers, and formatters. The web suite uses
Vitest for the same core behavior plus model and graph-chaining tests.

## Controls

- **2D / 3D**: switch graph mode. The plane selector (`XY`, `XZ`, `YZ`) appears
  in 2D mode.
- **i, j, k / Column**: change input and display format for all vectors.
- **Display**: toggle axis numbers, vector labels, and head-to-tail chaining.
- **Dark mode**: switch themes.
- **Fit**: frame the visible vectors and operation result.
- **+ Add**: add a vector.
- **×**: delete a vector.
- **Checkbox**: show or hide a vector on the graph.
- **Name field**: rename a vector; expressions update automatically.
- **3D graph**: left-drag to orbit, scroll to zoom.
- **2D graph**: left-drag to pan, scroll to zoom around the cursor.
- **Save as new vector**: add the operation result to the vector list.

## Expression grammar

```
expr    := term (('+' | '-') term)*
term    := primary (('.' | '·' | '*' | 'x' | 'X' | '×') primary)*
primary := NAME | '(' expr ')'
```

`+` and `-` combine vectors; `.`/`·`/`*` return a scalar; `x`/`X`/`×` return a
vector. Dot and cross have equal precedence and associate left to right. The
standalone `x` operator must be separated by spaces (`A x B`); `.` may be
adjacent (`A.B`). Vector names must start with a letter or underscore and contain
only letters, digits, and underscores. `i`, `j`, `k`, `x`, `X`, `dot`, and
`cross` are reserved.

## Project layout

```
CMakeLists.txt          Desktop build
resources.qrc           Qt resources
assets/check.png        Checkbox icon
src/                    Qt application sources
tests/                  C++ tests
web/
  index.html            Web entry page
  src/core/             Vector math, parser, model, themes
  src/components/       React UI and graph views
  src/*.test.ts         Vitest suites
  public/favicon.svg    Web icon
.github/workflows/
  deploy-web.yml        GitHub Pages deployment
```

## License

MIT
