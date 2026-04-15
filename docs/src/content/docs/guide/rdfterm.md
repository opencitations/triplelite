---
title: Data model
description: How LiteGraph represents triples, URIs, and literals
---

A triple in LiteGraph is a Python tuple of three elements:

```python
(subject: str, predicate: str, object: RDFTerm)
```

Subjects and predicates are plain URI strings. Blank nodes are not supported in any position. Objects are `RDFTerm` instances, which can represent either a URI or a literal:

```python
class RDFTerm(NamedTuple):
    type: str       # "uri" or "literal"
    value: str      # URI string or literal text
    datatype: str   # XSD datatype URI (default "")
    lang: str       # language tag (default "")
```

`datatype` and `lang` only apply to literals. A typed literal carries a datatype URI; a language-tagged literal carries a tag and no datatype:

```python
from litegraph import RDFTerm

uri = RDFTerm("uri", "http://purl.org/spar/fabio/JournalArticle")
year = RDFTerm("literal", "2020", "http://www.w3.org/2001/XMLSchema#gYear")
label_it = RDFTerm("literal", "Articolo di rivista", "", "it")
label_en = RDFTerm("literal", "Journal article", "", "en")
```
