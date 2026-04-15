# LiteGraph

Lightweight in-memory RDF triple store for Python with configurable indexing.

[![PyPI](https://img.shields.io/pypi/v/litegraph)](https://pypi.org/project/litegraph/)
[![Python](https://img.shields.io/pypi/pyversions/litegraph)](https://pypi.org/project/litegraph/)
[![Tests](https://img.shields.io/github/actions/workflow/status/opencitations/litegraph/test.yml?label=tests)](https://github.com/opencitations/litegraph/actions/workflows/test.yml)
[![Coverage](https://opencitations.github.io/litegraph/coverage-badge.svg)](https://opencitations.github.io/litegraph/coverage/)
[![REUSE](https://api.reuse.software/badge/github.com/opencitations/litegraph)](https://api.reuse.software/info/github.com/opencitations/litegraph)
[![License: ISC](https://img.shields.io/badge/license-ISC-blue.svg)](LICENSES/ISC.txt)

## Install

```sh
pip install litegraph
```

For [rdflib](https://rdflib.readthedocs.io/) interop:

```sh
pip install litegraph[rdflib]
```

## Quick start

```python
from litegraph import LiteGraph, RDFTerm

g = LiteGraph()
g.add(("http://example.org/s", "http://example.org/p", RDFTerm("uri", "http://example.org/o")))

for s, p, o in g.triples(("http://example.org/s", None, None)):
    print(s, p, o.value)
```

See the full documentation at [opencitations.github.io/litegraph](https://opencitations.github.io/litegraph).

## License

[ISC](LICENSES/ISC.txt)
