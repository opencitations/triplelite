---
title: Contributing
description: Development setup and working with the Python/C codebase
---

TripleLite has a Python layer (`triplelite/_graph.py`, `_types.py`, `_rdflib_bridge.py`) that sits on top of a C extension (`triplelite/_core.c` and its supporting modules). The C code handles storage, indexing, and lookup; Python handles the public API, type conversions, and rdflib bridging.

## Prerequisites

- Python 3.10+
- A C compiler (gcc or clang)
- [uv](https://docs.astral.sh/uv/) for dependency management
- Meson and Ninja (listed in dev dependencies, installed by `uv sync`)

## Setup

```sh
git clone https://github.com/opencitations/triplelite.git
cd triplelite
uv sync
uv pip install --no-build-isolation -e .
```

The editable install (`-e .`) invokes Meson to compile the C extension in place. After changing any `.c` or `.h` file, re-run:

```sh
uv pip install --no-build-isolation -e .
```

There is no separate `make` step; Meson handles the compilation through the pip install.

## Running tests

```sh
uv run pytest
```

Tests live in `tests/` and use pytest with Hypothesis for property-based testing. Type checking:

```sh
uv run pyright
```
