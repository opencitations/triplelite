## [1.3.1](https://github.com/opencitations/triplelite/compare/v1.3.0...v1.3.1) (2026-05-03)


### Bug Fixes

* **ci:** drop windows ARM64 wheel builds [release] ([4c6d37d](https://github.com/opencitations/triplelite/commit/4c6d37d10b669e67ba85de266372092aebce8037))

# [1.3.0](https://github.com/opencitations/triplelite/compare/v1.2.0...v1.3.0) (2026-05-03)


### Bug Fixes

* support Python 3.11 by including structmember.h ([87ca533](https://github.com/opencitations/triplelite/commit/87ca53357a4aeb653cb002c3c3cf753ab9c4a701))
* use-after-free in init and missing input validation in add_many ([c8dc8e4](https://github.com/opencitations/triplelite/commit/c8dc8e414738b20437ca13578882a2d3c09c4cf2))


### Features

* add a chained hashmap (djb2) ([81137f5](https://github.com/opencitations/triplelite/commit/81137f59cf34e2b134bec2a2166912e9c043ed10))
* add integer set and SPO triple index ([2a40d65](https://github.com/opencitations/triplelite/commit/2a40d65c7f7a51e6cc9b2e94553193af10468b6f))
* add RDFTerm hashmap and string/RDFTerm interners ([5e8256a](https://github.com/opencitations/triplelite/commit/5e8256add7fc08d7efddc2a527165bb1264f848a))
* delegate core TripleLite operations to C extension ([c0ff67b](https://github.com/opencitations/triplelite/commit/c0ff67be9e7066e6821da01082697aa140cb63c7))
* implement TripleLite C extension with Python bindings ([606d2d4](https://github.com/opencitations/triplelite/commit/606d2d4383a7f9a629a892496e0f01d2b2996239))
* move POS index, identifier, and add_many to C extension. ([95373ca](https://github.com/opencitations/triplelite/commit/95373ca7b67297f3dbbd759a9a6b353f99f4a6ed))


### Performance Improvements

* add dynamic resizing to all hash tables and optimize query lookups ([d266629](https://github.com/opencitations/triplelite/commit/d266629b0fac9bca3ca5dc668e226a0c162ea673))
* replace chained hash tables with open-addressing ([b0bc371](https://github.com/opencitations/triplelite/commit/b0bc371004598daf618581208ea27111c74e140f))

<!--
SPDX-FileCopyrightText: 2026 Arcangelo Massari <arcangelo.massari@unibo.it>

SPDX-License-Identifier: CC0-1.0
-->

# [1.2.0](https://github.com/opencitations/triplelite/compare/v1.1.0...v1.2.0) (2026-04-16)


### Features

* replace subgraph copy with zero-copy SubgraphView [release] ([4326f1a](https://github.com/opencitations/triplelite/commit/4326f1ad9c70b683a4de245772b0f01933745377))

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
