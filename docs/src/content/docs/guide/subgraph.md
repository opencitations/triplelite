---
title: Subgraph extraction
description: Extracting all triples for a subject into a new graph
---

## Checking if a subject exists

`has_subject()` returns `True` if the graph contains at least one triple with the given subject. It is an O(1) lookup on the SPO index:

```python
from triplelite import TripleLite, RDFTerm

g = TripleLite()
g.add(("http://example.org/a", "http://example.org/p", RDFTerm("uri", "http://example.org/b")))

g.has_subject("http://example.org/a")  # True
g.has_subject("http://example.org/z")  # False
```

## Extracting a subgraph

`subgraph()` pulls all triples for a given subject into a new, independent `TripleLite`:

```python
sub = g.subgraph(f"{OC}br/062501777134")
if sub is not None:
    for s, p, o in sub:
        print(p, o.value)
```

Returns `None` if the subject has no triples. Modifying the returned graph does not affect the original.
