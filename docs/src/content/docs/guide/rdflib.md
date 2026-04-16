---
title: rdflib interop
description: Converting between TripleLite and rdflib
---

Requires the `rdflib` extra (`pip install triplelite[rdflib]`).

## Converting rdflib terms

`rdflib_to_rdfterm()` converts an rdflib `URIRef`, `Literal`, or `BNode` into an `RDFTerm`. If the input is already an `RDFTerm`, it is returned as-is. Untyped literals default to `xsd:string`:

```python
from rdflib import URIRef, Literal, XSD
from triplelite import rdflib_to_rdfterm

rdflib_to_rdfterm(URIRef("http://example.org/x"))
# RDFTerm(type='uri', value='http://example.org/x', datatype='', lang='')

rdflib_to_rdfterm(Literal("ciao", lang="it"))
# RDFTerm(type='literal', value='ciao', datatype='', lang='it')

rdflib_to_rdfterm(Literal(42, datatype=XSD.integer))
# RDFTerm(type='literal', value='42', datatype='http://www.w3.org/2001/XMLSchema#integer', lang='')
```

## Exporting to rdflib

`to_rdflib()` converts a TripleLite to an rdflib object. If the graph has an identifier, it returns a `Dataset` (quads). Otherwise, it returns a `Graph` (triples). Also available as a method on `TripleLite`:

```python
from triplelite import TripleLite, RDFTerm

g = TripleLite(identifier="https://w3id.org/oc/meta/br/")
g.add(("https://w3id.org/oc/meta/br/062501777134",
       "http://purl.org/dc/terms/title",
       RDFTerm("literal", "OpenCitations, An Infrastructure Organization For Open Scholarship")))

ds = g.to_rdflib()  # Dataset, because identifier is set
print(ds.serialize(format="nquads"))
```

## Importing from rdflib

`from_rdflib()` accepts either an rdflib `Graph` or `Dataset` and returns a list of `TripleLite` instances:

```python
from rdflib import Graph, Dataset
from triplelite import from_rdflib

rg = Graph(identifier="https://w3id.org/oc/meta/br/")
rg.parse("data.ttl")
graphs = from_rdflib(rg)

ds = Dataset()
ds.parse("data.nq", format="nquads")
graphs = from_rdflib(ds)
```
