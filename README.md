# TripleLite

Lightweight in-memory RDF triple store for Python with configurable indexing.

[![PyPI](https://img.shields.io/pypi/v/triplelite)](https://pypi.org/project/triplelite/)
[![Python](https://img.shields.io/pypi/pyversions/triplelite)](https://pypi.org/project/triplelite/)
[![Tests](https://img.shields.io/github/actions/workflow/status/opencitations/triplelite/test.yml?label=tests)](https://github.com/opencitations/triplelite/actions/workflows/test.yml)
[![Coverage](https://opencitations.github.io/triplelite/coverage-badge.svg)](https://opencitations.github.io/triplelite/coverage/)
[![REUSE](https://api.reuse.software/badge/github.com/opencitations/triplelite)](https://api.reuse.software/info/github.com/opencitations/triplelite)
[![License: ISC](https://img.shields.io/badge/license-ISC-blue.svg)](LICENSES/ISC.txt)

## Install

```sh
pip install triplelite
```

For [rdflib](https://rdflib.readthedocs.io/) interop:

```sh
pip install triplelite[rdflib]
```

## Quick start

```python
from triplelite import TripleLite, RDFTerm

g = TripleLite()
g.add(("http://example.org/s", "http://example.org/p", RDFTerm("uri", "http://example.org/o")))

for s, p, o in g.triples(("http://example.org/s", None, None)):
    print(s, p, o.value)
```

See the full documentation at [opencitations.github.io/triplelite](https://opencitations.github.io/triplelite).

## License

[ISC](LICENSES/ISC.txt)
