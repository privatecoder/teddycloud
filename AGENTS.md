# Repository Guidelines

This repository is a TeddyCloud fork: a local Toniebox server for self-hosting cloud services, limiting vendor-cloud traffic, and serving figurine audio from local storage such as a NAS. Contributions should prioritize reliability, bug fixes, and incremental improvements over broad rewrites.

## Project Structure & Module Organization
Core server code lives in `src/`, with platform code in `src/platform/` and generated protobuf sources in `src/proto/`. Public headers are in `include/`. Third-party libraries such as `cyclone/`, `cJSON/`, `ogg/`, `opus/`, and `fat/` are vendored in-tree. Runtime assets ship from `contrib/`. The web frontend lives in the `teddycloud_web/` submodule and is copied into `contrib/data/www/web/` during builds. Tests currently live in `tests/`.

## Build, Test, and Development Commands
Clone with submodules: `git clone --recurse-submodules ...`.

- `make` or `make all`: checks dependencies, updates submodules, builds the web app, then compiles the server.
- `make build`: compile the native `bin/teddycloud` binary without rebuilding web assets.
- `make web`: install frontend dependencies in `teddycloud_web/`, build the UI, and refresh bundled web files.
- `make zip`: create `install/zip/release.zip` from a preinstall layout.
- `make scan-build`: run Clang static analysis and write HTML output to `report/`.
- `make cppcheck`: run `cppcheck` and emit `cppcheck.xml` plus an HTML report.
- `python3 tests/requests.py`: smoke-test the HTTP endpoints on a running local instance.

## Coding Style & Naming Conventions
Follow the existing C style: 4-space indentation, opening braces on their own line, and `snake_case` for functions and variables. Keep macros uppercase, for example `DEFAULT_HTTP_PORT`. Prefer small, focused fixes inside existing module boundaries. No repository-wide formatter config is checked in, so match surrounding code exactly.

## Testing Guidelines
Before opening a PR, run `make build` and, for HTTP or frontend changes, run `python3 tests/requests.py` against a local server. Use `make scan-build` or `make cppcheck` for risky C changes. There is no published coverage gate in CI, so add focused tests where behavior can be exercised automatically.

## Commit & Pull Request Guidelines
Target `develop`, not `master`; CI and repo policy expect `develop` as the base branch. Recent history favors short, imperative commit subjects such as `Add two tb2 settings`, `style: remove TRACE prefix tags per review`, or `Update frontend (make web)`. Keep commits narrow and descriptive. PRs should explain the change, note any build or runtime impact, link related issues, and include screenshots when frontend output changes.
