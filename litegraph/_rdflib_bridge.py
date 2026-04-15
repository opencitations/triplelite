# SPDX-FileCopyrightText: 2026 Arcangelo Massari <arcangelo.massari@unibo.it>
#
# SPDX-License-Identifier: ISC

from __future__ import annotations

from typing import TYPE_CHECKING

from rdflib import BNode, Dataset, Graph, Literal, Node, URIRef

from litegraph._types import _XSD_STRING, RDFTerm

if TYPE_CHECKING:
    from litegraph._graph import LiteGraph


def _rdflib_to_rdfterm(node: URIRef | Literal | Node | RDFTerm) -> RDFTerm:
    if isinstance(node, RDFTerm):
        return node
    if isinstance(node, Literal):
        language = str(node.language) if node.language else ""
        datatype = "" if language else (str(node.datatype) if node.datatype else _XSD_STRING)
        return RDFTerm("literal", str(node), datatype, language)
    return RDFTerm("uri", str(node))


def _to_rdflib_triple(obj: RDFTerm) -> URIRef | Literal:
    if obj.type == "literal":
        if obj.lang:
            return Literal(obj.value, lang=obj.lang)
        return Literal(obj.value, datatype=URIRef(obj.datatype))
    return URIRef(obj.value)


def _from_rdflib_graph(graph: Graph) -> LiteGraph:
    from litegraph._graph import LiteGraph

    identifier = str(graph.identifier) if not isinstance(graph.identifier, BNode) else None
    litegraph = LiteGraph(identifier=identifier)
    for subject, predicate, obj in graph:
        litegraph.add((str(subject), str(predicate), _rdflib_to_rdfterm(obj)))
    return litegraph


def from_rdflib(source: Graph | Dataset) -> list[LiteGraph]:
    if isinstance(source, Dataset):
        return [_from_rdflib_graph(graph) for graph in source.graphs()]
    return [_from_rdflib_graph(source)]


def to_rdflib(source: LiteGraph) -> Graph | Dataset:
    if source.identifier is not None:
        dataset = Dataset()
        named_graph = dataset.graph(URIRef(source.identifier))
        for s, p, o in source:
            named_graph.add((URIRef(s), URIRef(p), _to_rdflib_triple(o)))
        return dataset
    graph = Graph()
    for s, p, o in source:
        graph.add((URIRef(s), URIRef(p), _to_rdflib_triple(o)))
    return graph
