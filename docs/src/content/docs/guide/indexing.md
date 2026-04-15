---
title: Reverse indexing
description: SPO and POS indices, configuration, and memory trade-offs
---

By default, TripleLite builds a single index mapping subjects to their predicates and objects (SPO). Queries that start from a subject are a dictionary lookup. Queries that start from an object scan the entire graph.

Passing `reverse_index_predicates` at construction time adds a second index that maps predicates and objects back to subjects (POS). This makes `subjects()` a direct lookup instead of a scan, at the cost of roughly doubling memory and slowing down writes.

Two modes:

```python
from triplelite import TripleLite, RDFTerm

FRBR_PART_OF = "http://purl.org/vocab/frbr/core#partOf"
RDF_TYPE = "http://www.w3.org/1999/02/22-rdf-syntax-ns#type"

# Index only the predicates you query by object
g = TripleLite(reverse_index_predicates=frozenset({FRBR_PART_OF, RDF_TYPE}))

# Index every predicate (empty frozenset = all)
g = TripleLite(reverse_index_predicates=frozenset())
```

Selective indexing is the typical choice. If you load a scholarly graph and need to answer "which articles belong to this journal?", index `frbr:partOf`. If you also need "which resources are journal articles?", add `rdf:type`. Everything else stays in SPO only.

The index must be set at construction time. Triples added before the index is configured are not retroactively indexed.

## Example: finding articles in a journal

```python
from triplelite import TripleLite, RDFTerm

FABIO = "http://purl.org/spar/fabio/"
FRBR = "http://purl.org/vocab/frbr/core#"
RDF_TYPE = "http://www.w3.org/1999/02/22-rdf-syntax-ns#type"
OC = "https://w3id.org/oc/meta/"

g = TripleLite(reverse_index_predicates=frozenset({f"{FRBR}partOf"}))

journal = f"{OC}br/062501778099"
for omid in ("062501777134", "062504927381", "062507283910"):
    article = f"{OC}br/{omid}"
    g.add((article, RDF_TYPE, RDFTerm("uri", f"{FABIO}JournalArticle")))
    g.add((article, f"{FRBR}partOf", RDFTerm("uri", journal)))

# Direct lookup via reverse index, no scan
for s in g.subjects(predicate=f"{FRBR}partOf",
                    object=RDFTerm("uri", journal)):
    print(s)
```
