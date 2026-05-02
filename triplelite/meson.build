py.extension_module(
    '_core',
    '_core.c',
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
    '_graph.py',
    '_rdflib_bridge.py',
    '_types.py',
    'py.typed',
    subdir: 'triplelite',
)
