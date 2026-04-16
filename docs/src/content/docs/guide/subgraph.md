---
title: Subgraph extraction
description: Extracting a zero-copy view of all triples for a subject
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

`subgraph()` returns a `SubgraphView`: a read-only, zero-copy projection of all triples for a given subject. The view references the parent graph's internal data structures directly, with no copying:

```python
sub = g.subgraph(f"{OC}br/062501777134")
if sub is not None:
    for s, p, o in sub:
        print(p, o.value)
```

Returns `None` if the subject has no triples.

## SubgraphView is a live view

Because `SubgraphView` points to the parent's data, it reflects changes made to the parent after creation:

```python
g = TripleLite()
g.add(("http://example.org/a", "http://example.org/p1", RDFTerm("uri", "http://example.org/b")))
sub = g.subgraph("http://example.org/a")

g.add(("http://example.org/a", "http://example.org/p2", RDFTerm("uri", "http://example.org/c")))
len(sub)  # 2 (includes the triple added after view creation)
```

## Supported operations

`SubgraphView` supports iteration, length, predicate-object queries, and set operations for change detection:

```python
current_triples = set(entity.g)
removed = sub - current_triples   # triples in view but not in current
added = current_triples - sub     # triples in current but not in view
sub == current_triples            # equality check
```
