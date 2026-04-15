---
title: Subgraph extraction
description: Extracting all triples for a subject into a new graph
---

`subgraph()` pulls all triples for a given subject into a new, independent `LiteGraph`:

```python
sub = g.subgraph(f"{OC}br/062501777134")
if sub is not None:
    for s, p, o in sub:
        print(p, o.value)
```

Returns `None` if the subject has no triples. Modifying the returned graph does not affect the original.
