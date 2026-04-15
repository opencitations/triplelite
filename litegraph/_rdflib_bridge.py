# SPDX-FileCopyrightText: 2026 Arcangelo Massari <arcangelo.massari@unibo.it>
#
# SPDX-License-Identifier: ISC

from __future__ import annotations

from typing import TYPE_CHECKING, Iterator

from rdflib import BNode, Dataset, Graph, Literal, Node, URIRef

from litegraph._types import _XSD_STRING, RDFTerm

if TYPE_CHECKING:
    from litegraph._graph import LiteGraph


def rdflib_to_rdfterm(node: URIRef | Literal | Node | RDFTerm) -> RDFTerm:
    if isinstance(node, RDFTerm):
        return node
    if isinstance(node, Literal):
        language = str(node.language) if node.language else ""
        datatype = "" if language else (str(node.datatype) if node.datatype else _XSD_STRING)
        return RDFTerm("literal", str(node), datatype, language)
    return RDFTerm("uri", str(node))


def to_rdflib_quads(graph: LiteGraph) -> Iterator[tuple]:
    graph_id = URIRef(graph.identifier) if graph.identifier else None
    for subject, predicate, obj in graph:
        subject_ref = URIRef(subject)
        predicate_ref = URIRef(predicate)
        if obj.type == "literal":
            if obj.lang:
                object_ref = Literal(obj.value, lang=obj.lang)
            else:
                object_ref = Literal(obj.value, datatype=URIRef(obj.datatype))
        else:
            object_ref = URIRef(obj.value)
        yield subject_ref, predicate_ref, object_ref, graph_id


def from_rdflib_graph(graph: Graph) -> LiteGraph:
    from litegraph._graph import LiteGraph

    identifier = str(graph.identifier) if not isinstance(graph.identifier, BNode) else None
    litegraph = LiteGraph(identifier=identifier)
    for subject, predicate, obj in graph:
        litegraph.add((str(subject), str(predicate), rdflib_to_rdfterm(obj)))
    return litegraph


def from_rdflib_dataset(dataset: Dataset) -> list[LiteGraph]:
    return [from_rdflib_graph(graph) for graph in dataset.graphs()]


def to_rdflib_dataset(graph: LiteGraph) -> Dataset:
    dataset = Dataset()
    if graph.identifier is not None:
        dataset.addN(to_rdflib_quads(graph))
    else:
        default_graph = dataset.default_graph
        for subject, predicate, obj, _ctx in to_rdflib_quads(graph):
            default_graph.add((subject, predicate, obj))
    return dataset
