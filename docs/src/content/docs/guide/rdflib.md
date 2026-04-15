---
title: rdflib interop
description: Converting between LiteGraph and rdflib
---

Requires the `rdflib` extra (`pip install litegraph[rdflib]`).

## Converting rdflib terms

`rdflib_to_rdfterm()` turns `URIRef` and `Literal` into `RDFTerm`. If you already have an `RDFTerm`, it passes through unchanged:

```python
from rdflib import URIRef, Literal
from litegraph import rdflib_to_rdfterm

rdflib_to_rdfterm(URIRef("http://purl.org/spar/fabio/JournalArticle"))
# RDFTerm(type='uri', value='http://purl.org/spar/fabio/JournalArticle', ...)

rdflib_to_rdfterm(Literal("2020-02", datatype=URIRef("http://www.w3.org/2001/XMLSchema#gYearMonth")))
# RDFTerm(type='literal', value='2020-02', datatype='...gYearMonth', lang='')
```

## Exporting to an rdflib Dataset

`to_rdflib_dataset()` converts a LiteGraph into an rdflib `Dataset`. Also available as a method on `LiteGraph`:

```python
from litegraph import LiteGraph, RDFTerm

g = LiteGraph(identifier="https://w3id.org/oc/meta/br/")
g.add(("https://w3id.org/oc/meta/br/062501777134",
       "http://purl.org/dc/terms/title",
       RDFTerm("literal", "OpenCitations, An Infrastructure Organization For Open Scholarship")))

ds = g.to_rdflib_dataset()
print(ds.serialize(format="nquads"))
```

## Importing from rdflib

`from_rdflib_graph()` converts a single rdflib `Graph` into a `LiteGraph`:

```python
from rdflib import Graph
from litegraph import from_rdflib_graph

rg = Graph(identifier="https://w3id.org/oc/meta/br/")
rg.parse("data.ttl")

lg = from_rdflib_graph(rg)
```

`from_rdflib_dataset()` converts an rdflib `Dataset` into a list of `LiteGraph` instances, one per named graph:

```python
from rdflib import Dataset
from litegraph import from_rdflib_dataset

ds = Dataset()
ds.parse("data.nq", format="nquads")

graphs = from_rdflib_dataset(ds)
```
