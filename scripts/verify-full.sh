#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${REPO_ROOT}"

echo "== Submodules =="
git submodule update --init --recursive
git submodule status

echo "== Build =="
make build

echo "== Cppcheck =="
make cppcheck

echo "== Scan Build =="
make scan-build
