---
title: TripleLite
description: Lightweight in-memory RDF graph for Python with configurable indexing
template: doc
---

TripleLite is an in-memory RDF triple store for Python. It stores triples in nested dictionaries, with an optional reverse index configurable per predicate.

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

g.add((article, RDF_TYPE, RDFTerm("uri", f"{FABIO}JournalArticle")))
g.add((article, f"{DCTERMS}title",
       RDFTerm("literal", "OpenCitations, An Infrastructure Organization For Open Scholarship")))
g.add((article, f"{PRISM}publicationDate",
       RDFTerm("literal", "2020-02", f"{XSD}gYearMonth")))
g.add((article, f"{PRISM}volume", RDFTerm("literal", "1")))
g.add((article, f"{PRISM}startingPage", RDFTerm("literal", "428")))
g.add((article, f"{PRISM}endingPage", RDFTerm("literal", "444")))
g.add((article, f"{FRBR}partOf", RDFTerm("uri", journal)))
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

## What's next

- [Data model](/triplelite/guide/rdfterm/): how triples, URIs, and literals are represented internally
- [Reverse indexing](/triplelite/guide/indexing/): when to enable it, selective vs full, memory trade-offs
- [Subgraph extraction](/triplelite/guide/subgraph/): pulling all triples for a subject into a new graph
- [rdflib interop](/triplelite/guide/rdflib/): converting between TripleLite and rdflib
