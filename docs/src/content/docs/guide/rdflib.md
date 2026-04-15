---
title: rdflib interop
description: Converting between LiteGraph and rdflib
---

Requires the `rdflib` extra (`pip install litegraph[rdflib]`).

## Exporting to rdflib

`to_rdflib()` converts a LiteGraph to an rdflib object. If the graph has an identifier, it returns a `Dataset` (quads). Otherwise, it returns a `Graph` (triples). Also available as a method on `LiteGraph`:

```python
from litegraph import LiteGraph, RDFTerm

g = LiteGraph(identifier="https://w3id.org/oc/meta/br/")
g.add(("https://w3id.org/oc/meta/br/062501777134",
       "http://purl.org/dc/terms/title",
       RDFTerm("literal", "OpenCitations, An Infrastructure Organization For Open Scholarship")))

ds = g.to_rdflib()  # Dataset, because identifier is set
print(ds.serialize(format="nquads"))
```

## Importing from rdflib

`from_rdflib()` accepts either an rdflib `Graph` or `Dataset` and returns a list of `LiteGraph` instances:

```python
from rdflib import Graph, Dataset
from litegraph import from_rdflib

rg = Graph(identifier="https://w3id.org/oc/meta/br/")
rg.parse("data.ttl")
graphs = from_rdflib(rg)

ds = Dataset()
ds.parse("data.nq", format="nquads")
graphs = from_rdflib(ds)
```
