---
title: TripleLite
description: In-memory RDF triple store for Python, optimized for speed and memory efficiency with a C core
template: doc
---

TripleLite is an in-memory RDF triple store for Python, optimized for speed and memory efficiency. The storage engine is written in C, with a Python API on top. It supports configurable reverse indexing per predicate.

## Install

```sh
pip install triplelite
```

Need [rdflib](https://rdflib.readthedocs.io/) interop? Install the extra:

```sh
pip install triplelite[rdflib]
```

## Quick start

This example models the article ["OpenCitations, an infrastructure organization for open scholarship"](https://doi.org/10.1162/qss_a_00023) using predicates from the [OpenCitations Data Model](https://doi.org/10.6084/m9.figshare.3443876):

```python
from triplelite import TripleLite, RDFTerm

DCTERMS = "http://purl.org/dc/terms/"
FABIO = "http://purl.org/spar/fabio/"
FRBR = "http://purl.org/vocab/frbr/core#"
PRISM = "http://prismstandard.org/namespaces/basic/2.0/"
XSD = "http://www.w3.org/2001/XMLSchema#"
RDF_TYPE = "http://www.w3.org/1999/02/22-rdf-syntax-ns#type"

OC = "https://w3id.org/oc/meta/"
article = f"{OC}br/062501777134"
journal = f"{OC}br/062501778099"

g = TripleLite(identifier=f"{OC}br/")

g.add_many([
    (article, RDF_TYPE, RDFTerm("uri", f"{FABIO}JournalArticle")),
    (article, f"{DCTERMS}title",
     RDFTerm("literal", "OpenCitations, An Infrastructure Organization For Open Scholarship")),
    (article, f"{PRISM}publicationDate",
     RDFTerm("literal", "2020-02", f"{XSD}gYearMonth")),
    (article, f"{PRISM}volume", RDFTerm("literal", "1")),
    (article, f"{PRISM}startingPage", RDFTerm("literal", "428")),
    (article, f"{PRISM}endingPage", RDFTerm("literal", "444")),
    (article, f"{FRBR}partOf", RDFTerm("uri", journal)),
])
```

Query by pattern (`None` is a wildcard):

```python
for s, p, o in g.triples((article, None, None)):
    print(p.split("/")[-1], "->", o.value)

for obj in g.objects(article, f"{DCTERMS}title"):
    print(obj.value)

print(len(g))  # 7
```

Remove by pattern:

```python
g.remove((article, f"{PRISM}startingPage", None))
g.remove((article, f"{PRISM}endingPage", None))
print(len(g))  # 5
```

## Single add vs batch add

`add()` inserts one triple. `add_many()` accepts any iterable and avoids per-call overhead by caching internal lookups across the batch. Use `add_many()` when loading data from a file, a SPARQL result set, or any source with more than a handful of triples:

```python
g.add((article, f"{DCTERMS}title", RDFTerm("literal", "Some title")))

g.add_many(
    (str(s), str(p), RDFTerm("uri", str(o)))
    for s, p, o in some_external_source
)
```

Both methods deduplicate: adding a triple that already exists is a no-op.

## What's next

- [Data model](/triplelite/guide/rdfterm/): how triples, URIs, and literals are represented internally
- [Reverse indexing](/triplelite/guide/indexing/): when to enable it, selective vs full, memory trade-offs
- [Subgraph extraction](/triplelite/guide/subgraph/): zero-copy views of all triples for a subject
- [rdflib interop](/triplelite/guide/rdflib/): converting between TripleLite and rdflib
- [Contributing](/triplelite/guide/contributing/): development setup, project layout, working with Python and C
