# [1.1.0](https://github.com/opencitations/triplelite/compare/v1.0.0...v1.1.0) (2026-04-16)


### Features

* expose public API for rdflib_to_rdfterm, XSD_STRING, and has_subject [release] ([59059e4](https://github.com/opencitations/triplelite/commit/59059e41b847e7e4e2c9ea2f0b304ab7908b92fb))

# 1.0.0 (2026-04-15)


* refactor!: rename package from litegraph to triplelite ([f889b86](https://github.com/opencitations/triplelite/commit/f889b865846d21590e6a7e7e3078d052352655dc))
* refactor!: unify rdflib bridge into from_rdflib and to_rdflib ([d78bf2d](https://github.com/opencitations/triplelite/commit/d78bf2de98b8cead4937ccc8ca68c404c5c844cd))


### Features

* add in-memory RDF triple store with configurable indexing ([be25f2a](https://github.com/opencitations/triplelite/commit/be25f2ab305aada3b94dd6477e563f2075f9a230))


### Performance Improvements

* intern strings and terms as integer IDs in internal indices ([b631e9a](https://github.com/opencitations/triplelite/commit/b631e9abec4bb9c47605455a13da5f5dbb43b502))


### BREAKING CHANGES

* package renamed from litegraph to triplelite and main
class from LiteGraph to TripleLite. Update all imports accordingly.
* removed public functions rdflib_to_rdfterm,
from_rdflib_graph, from_rdflib_dataset, to_rdflib_dataset, and
to_rdflib_quads. LiteGraph methods to_rdflib_dataset and
to_rdflib_quads replaced by to_rdflib. from_rdflib now returns
list[LiteGraph] even for a single Graph input.
