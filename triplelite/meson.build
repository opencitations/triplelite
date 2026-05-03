# SPDX-FileCopyrightText: 2026 Arcangelo Massari <arcangelo.massari@unibo.it>
#
# SPDX-License-Identifier: CC0-1.0

py.extension_module(
    '_core',
    '_core.c',
    'oa_hash.c',
    'dynarray.c',
    'hashmap.c',
    'rdfterm_hashmap.c',
    'intset.c',
    'spo_index.c',
    subdir: 'triplelite',
    install: true,
)

py.install_sources(
    '__init__.py',
    '_core.pyi',
    '_graph.py',
    '_rdflib_bridge.py',
    '_types.py',
    'py.typed',
    subdir: 'triplelite',
)
