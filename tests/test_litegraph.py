# SPDX-FileCopyrightText: 2026 Arcangelo Massari <arcangelo.massari@unibo.it>
#
# SPDX-License-Identifier: ISC

from rdflib import XSD, Dataset, Graph, Literal, URIRef

from litegraph import (
    LiteGraph,
    RDFTerm,
    from_rdflib_dataset,
    from_rdflib_graph,
    rdflib_to_rdfterm,
    to_rdflib_dataset,
)

URI_A = "http://example.org/a"
URI_B = "http://example.org/b"
URI_C = "http://example.org/c"
PRED_1 = "http://example.org/p1"
PRED_2 = "http://example.org/p2"
PRED_3 = "http://example.org/p3"
OBJ_URI = RDFTerm("uri", "http://example.org/obj")
OBJ_LIT = RDFTerm("literal", "hello", "http://www.w3.org/2001/XMLSchema#string")
OBJ_LANG = RDFTerm("literal", "ciao", "", "it")
OBJ_INT = RDFTerm("literal", "42", "http://www.w3.org/2001/XMLSchema#integer")


class TestAdd:
    def test_add_single_triple(self):
        g = LiteGraph()
        g.add((URI_A, PRED_1, OBJ_URI))
        assert len(g) == 1

    def test_add_duplicate_ignored(self):
        g = LiteGraph()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_A, PRED_1, OBJ_URI))
        assert len(g) == 1

    def test_add_different_objects_same_sp(self):
        g = LiteGraph()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_A, PRED_1, OBJ_LIT))
        assert len(g) == 2

    def test_add_updates_reverse_index(self):
        g = LiteGraph(reverse_index_predicates=frozenset({PRED_1}))
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_B, PRED_1, OBJ_URI))
        assert set(g.subjects(PRED_1, OBJ_URI)) == {URI_A, URI_B}

    def test_add_selective_reverse_index(self):
        g = LiteGraph(reverse_index_predicates=frozenset({PRED_1}))
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_A, PRED_2, OBJ_LIT))
        assert g._pos is not None
        assert PRED_1 in g._pos
        assert PRED_2 not in g._pos

    def test_add_index_all_predicates(self):
        g = LiteGraph(reverse_index_predicates=frozenset())
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_A, PRED_2, OBJ_LIT))
        assert g._pos is not None
        assert PRED_1 in g._pos
        assert PRED_2 in g._pos

    def test_no_reverse_index_by_default(self):
        g = LiteGraph()
        g.add((URI_A, PRED_1, OBJ_URI))
        assert g._pos is None


class TestRemove:
    def test_remove_exact_triple(self):
        g = LiteGraph()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.remove((URI_A, PRED_1, OBJ_URI))
        assert len(g) == 0

    def test_remove_by_sp_wildcard(self):
        g = LiteGraph()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_A, PRED_1, OBJ_LIT))
        g.remove((URI_A, PRED_1, None))
        assert len(g) == 0

    def test_remove_by_subject_wildcard(self):
        g = LiteGraph()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_A, PRED_2, OBJ_LIT))
        g.remove((URI_A, None, None))
        assert len(g) == 0

    def test_remove_by_predicate_wildcard(self):
        g = LiteGraph()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_B, PRED_1, OBJ_LIT))
        g.remove((None, PRED_1, None))
        assert len(g) == 0

    def test_remove_by_object_wildcard(self):
        g = LiteGraph()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_B, PRED_2, OBJ_URI))
        g.remove((None, None, OBJ_URI))
        assert len(g) == 0

    def test_remove_all(self):
        g = LiteGraph()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_B, PRED_2, OBJ_LIT))
        g.remove((None, None, None))
        assert len(g) == 0
        assert g._spo == {}

    def test_remove_cleans_empty_dicts(self):
        g = LiteGraph()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.remove((URI_A, PRED_1, OBJ_URI))
        assert URI_A not in g._spo

    def test_remove_updates_reverse_index(self):
        g = LiteGraph(reverse_index_predicates=frozenset({PRED_1}))
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_B, PRED_1, OBJ_URI))
        g.remove((URI_A, PRED_1, OBJ_URI))
        assert set(g.subjects(PRED_1, OBJ_URI)) == {URI_B}

    def test_remove_all_clears_reverse_index(self):
        g = LiteGraph(reverse_index_predicates=frozenset({PRED_1}))
        g.add((URI_A, PRED_1, OBJ_URI))
        g.remove((None, None, None))
        assert g._pos == {}

    def test_remove_nonexistent_noop(self):
        g = LiteGraph()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.remove((URI_B, PRED_1, OBJ_URI))
        assert len(g) == 1


class TestTriples:
    def test_exact_match(self):
        g = LiteGraph()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_A, PRED_1, OBJ_LIT))
        result = list(g.triples((URI_A, PRED_1, OBJ_URI)))
        assert result == [(URI_A, PRED_1, OBJ_URI)]

    def test_sp_pattern(self):
        g = LiteGraph()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_A, PRED_1, OBJ_LIT))
        result = list(g.triples((URI_A, PRED_1, None)))
        assert len(result) == 2

    def test_s_pattern(self):
        g = LiteGraph()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_A, PRED_2, OBJ_LIT))
        result = list(g.triples((URI_A, None, None)))
        assert len(result) == 2

    def test_p_pattern(self):
        g = LiteGraph()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_B, PRED_1, OBJ_LIT))
        g.add((URI_A, PRED_2, OBJ_URI))
        result = list(g.triples((None, PRED_1, None)))
        assert len(result) == 2

    def test_o_pattern(self):
        g = LiteGraph()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_B, PRED_2, OBJ_URI))
        g.add((URI_A, PRED_1, OBJ_LIT))
        result = list(g.triples((None, None, OBJ_URI)))
        assert len(result) == 2

    def test_all_wildcard(self):
        g = LiteGraph()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_B, PRED_2, OBJ_LIT))
        result = list(g.triples((None, None, None)))
        assert len(result) == 2

    def test_no_match_returns_empty(self):
        g = LiteGraph()
        g.add((URI_A, PRED_1, OBJ_URI))
        result = list(g.triples((URI_B, PRED_1, None)))
        assert result == []

    def test_no_match_predicate(self):
        g = LiteGraph()
        g.add((URI_A, PRED_1, OBJ_URI))
        result = list(g.triples((URI_A, PRED_2, None)))
        assert result == []


class TestObjects:
    def test_sp_lookup(self):
        g = LiteGraph()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_A, PRED_1, OBJ_LIT))
        result = set(g.objects(URI_A, PRED_1))
        assert result == {OBJ_URI, OBJ_LIT}

    def test_subject_only(self):
        g = LiteGraph()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_A, PRED_2, OBJ_LIT))
        result = set(g.objects(subject=URI_A))
        assert result == {OBJ_URI, OBJ_LIT}

    def test_predicate_only(self):
        g = LiteGraph()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_B, PRED_1, OBJ_LIT))
        result = set(g.objects(predicate=PRED_1))
        assert result == {OBJ_URI, OBJ_LIT}

    def test_no_filter(self):
        g = LiteGraph()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_B, PRED_2, OBJ_LIT))
        result = set(g.objects())
        assert result == {OBJ_URI, OBJ_LIT}

    def test_empty_result(self):
        g = LiteGraph()
        assert list(g.objects(URI_A, PRED_1)) == []


class TestPredicateObjects:
    def test_with_subject(self):
        g = LiteGraph()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_A, PRED_2, OBJ_LIT))
        result = set(g.predicate_objects(URI_A))
        assert result == {(PRED_1, OBJ_URI), (PRED_2, OBJ_LIT)}

    def test_without_subject(self):
        g = LiteGraph()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_B, PRED_2, OBJ_LIT))
        result = set(g.predicate_objects())
        assert result == {(PRED_1, OBJ_URI), (PRED_2, OBJ_LIT)}


class TestSubjects:
    def test_with_reverse_index_po(self):
        g = LiteGraph(reverse_index_predicates=frozenset({PRED_1}))
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_B, PRED_1, OBJ_URI))
        g.add((URI_C, PRED_1, OBJ_LIT))
        assert set(g.subjects(PRED_1, OBJ_URI)) == {URI_A, URI_B}

    def test_with_reverse_index_p_only(self):
        g = LiteGraph(reverse_index_predicates=frozenset({PRED_1}))
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_B, PRED_1, OBJ_LIT))
        assert set(g.subjects(predicate=PRED_1)) == {URI_A, URI_B}

    def test_with_reverse_index_o_only(self):
        g = LiteGraph(reverse_index_predicates=frozenset({PRED_1, PRED_2}))
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_B, PRED_2, OBJ_URI))
        assert set(g.subjects(object=OBJ_URI)) == {URI_A, URI_B}

    def test_with_reverse_index_all_subjects(self):
        g = LiteGraph(reverse_index_predicates=frozenset({PRED_1}))
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_B, PRED_1, OBJ_LIT))
        assert set(g.subjects()) == {URI_A, URI_B}

    def test_without_reverse_index_fallback(self):
        g = LiteGraph()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_B, PRED_1, OBJ_URI))
        assert set(g.subjects(PRED_1, OBJ_URI)) == {URI_A, URI_B}

    def test_without_reverse_index_p_only(self):
        g = LiteGraph()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_B, PRED_2, OBJ_LIT))
        assert set(g.subjects(predicate=PRED_1)) == {URI_A}

    def test_without_reverse_index_o_only(self):
        g = LiteGraph()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_B, PRED_2, OBJ_URI))
        assert set(g.subjects(object=OBJ_URI)) == {URI_A, URI_B}

    def test_without_reverse_index_all(self):
        g = LiteGraph()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_B, PRED_2, OBJ_LIT))
        assert set(g.subjects()) == {URI_A, URI_B}

    def test_empty(self):
        g = LiteGraph(reverse_index_predicates=frozenset({PRED_1}))
        assert list(g.subjects(PRED_1, OBJ_URI)) == []

    def test_unindexed_predicate_not_in_pos(self):
        g = LiteGraph(reverse_index_predicates=frozenset({PRED_1}))
        g.add((URI_A, PRED_2, OBJ_URI))
        assert list(g.subjects(PRED_2, OBJ_URI)) == []


class TestSubgraph:
    def test_extracts_subject_triples(self):
        g = LiteGraph()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_A, PRED_2, OBJ_LIT))
        g.add((URI_B, PRED_1, OBJ_LIT))
        sub = g.subgraph(URI_A)
        assert sub is not None
        assert len(sub) == 2
        assert (URI_A, PRED_1, OBJ_URI) in sub
        assert (URI_A, PRED_2, OBJ_LIT) in sub

    def test_returns_none_for_missing(self):
        g = LiteGraph()
        assert g.subgraph(URI_A) is None

    def test_subgraph_no_reverse_index(self):
        g = LiteGraph()
        g.add((URI_A, PRED_1, OBJ_URI))
        sub = g.subgraph(URI_A)
        assert sub is not None
        assert sub._pos is None

    def test_subgraph_independent(self):
        g = LiteGraph()
        g.add((URI_A, PRED_1, OBJ_URI))
        sub = g.subgraph(URI_A)
        assert sub is not None
        sub.add((URI_A, PRED_2, OBJ_LIT))
        assert len(g) == 1
        assert len(sub) == 2


class TestContains:
    def test_contains_true(self):
        g = LiteGraph()
        g.add((URI_A, PRED_1, OBJ_URI))
        assert (URI_A, PRED_1, OBJ_URI) in g

    def test_contains_false(self):
        g = LiteGraph()
        g.add((URI_A, PRED_1, OBJ_URI))
        assert (URI_A, PRED_1, OBJ_LIT) not in g

    def test_contains_missing_subject(self):
        g = LiteGraph()
        assert (URI_A, PRED_1, OBJ_URI) not in g


class TestIter:
    def test_iter_all(self):
        g = LiteGraph()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_B, PRED_2, OBJ_LIT))
        result = set(g)
        assert result == {(URI_A, PRED_1, OBJ_URI), (URI_B, PRED_2, OBJ_LIT)}


class TestLen:
    def test_empty(self):
        assert len(LiteGraph()) == 0

    def test_after_adds(self):
        g = LiteGraph()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_A, PRED_2, OBJ_LIT))
        assert len(g) == 2

    def test_after_remove(self):
        g = LiteGraph()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_A, PRED_2, OBJ_LIT))
        g.remove((URI_A, PRED_1, OBJ_URI))
        assert len(g) == 1


class TestIdentifier:
    def test_default_none(self):
        assert LiteGraph().identifier is None

    def test_set_identifier(self):
        g = LiteGraph(identifier="http://example.org/graph")
        assert g.identifier == "http://example.org/graph"


class TestRdflibCompat:
    def test_to_rdflib_quads(self):
        g = LiteGraph(identifier="http://example.org/graph")
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_A, PRED_2, OBJ_LIT))
        g.add((URI_A, PRED_3, OBJ_LANG))

        quads = list(g.to_rdflib_quads())
        assert len(quads) == 3

        graph_id = URIRef("http://example.org/graph")
        for s, p, o, gid in quads:
            assert isinstance(s, URIRef)
            assert isinstance(p, URIRef)
            assert gid == graph_id

        obj_map = {str(p): o for s, p, o, gid in quads}
        assert obj_map[PRED_1] == URIRef("http://example.org/obj")
        assert obj_map[PRED_2] == Literal("hello", datatype=XSD.string)
        assert obj_map[PRED_3] == Literal("ciao", lang="it")

    def test_to_rdflib_quads_no_identifier(self):
        g = LiteGraph()
        g.add((URI_A, PRED_1, OBJ_URI))
        quads = list(g.to_rdflib_quads())
        assert quads[0][3] is None

    def test_rdflib_to_rdfterm_uri(self):
        term = rdflib_to_rdfterm(URIRef("http://example.org/x"))
        assert term == RDFTerm("uri", "http://example.org/x")

    def test_rdflib_to_rdfterm_literal(self):
        term = rdflib_to_rdfterm(Literal("test", datatype=XSD.string))
        assert term == RDFTerm("literal", "test", str(XSD.string))

    def test_rdflib_to_rdfterm_literal_lang(self):
        term = rdflib_to_rdfterm(Literal("ciao", lang="it"))
        assert term.lang == "it"
        assert term.type == "literal"

    def test_rdflib_to_rdfterm_passthrough(self):
        term = RDFTerm("uri", "http://x")
        assert rdflib_to_rdfterm(term) is term

    def test_rdflib_to_rdfterm_untyped_literal(self):
        term = rdflib_to_rdfterm(Literal("plain"))
        assert term.datatype == "http://www.w3.org/2001/XMLSchema#string"

    def test_to_rdflib_quads_integer_literal(self):
        g = LiteGraph()
        g.add((URI_A, PRED_1, OBJ_INT))
        quads = list(g.to_rdflib_quads())
        assert quads[0][2] == Literal("42", datatype=XSD.integer)

    def test_to_rdflib_dataset(self):
        g = LiteGraph(identifier="http://example.org/graph")
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_A, PRED_2, OBJ_LIT))
        ds = g.to_rdflib_dataset()
        assert len(ds) == 2
        assert (URIRef(URI_A), URIRef(PRED_1), URIRef("http://example.org/obj"), URIRef("http://example.org/graph")) in ds

    def test_to_rdflib_dataset_function(self):
        g = LiteGraph(identifier="http://example.org/graph")
        g.add((URI_A, PRED_1, OBJ_URI))
        ds = to_rdflib_dataset(g)
        assert len(ds) == 1

    def test_to_rdflib_dataset_no_identifier(self):
        g = LiteGraph()
        g.add((URI_A, PRED_1, OBJ_URI))
        ds = g.to_rdflib_dataset()
        assert len(ds) == 1

    def test_from_rdflib_graph(self):
        rg = Graph(identifier=URIRef("http://example.org/graph"))
        rg.add((URIRef(URI_A), URIRef(PRED_1), URIRef("http://example.org/obj")))
        rg.add((URIRef(URI_A), URIRef(PRED_2), Literal("hello", datatype=XSD.string)))
        lg = from_rdflib_graph(rg)
        assert lg.identifier == "http://example.org/graph"
        assert len(lg) == 2
        assert (URI_A, PRED_1, RDFTerm("uri", "http://example.org/obj")) in lg
        assert (URI_A, PRED_2, OBJ_LIT) in lg

    def test_from_rdflib_graph_no_identifier(self):
        rg = Graph()
        rg.add((URIRef(URI_A), URIRef(PRED_1), URIRef("http://example.org/obj")))
        lg = from_rdflib_graph(rg)
        assert lg.identifier is None
        assert len(lg) == 1

    def test_from_rdflib_graph_lang_literal(self):
        rg = Graph()
        rg.add((URIRef(URI_A), URIRef(PRED_1), Literal("ciao", lang="it")))
        lg = from_rdflib_graph(rg)
        assert (URI_A, PRED_1, OBJ_LANG) in lg

    def test_from_rdflib_dataset(self):
        ds = Dataset()
        ctx = ds.graph(URIRef("http://example.org/g1"))
        ctx.add((URIRef(URI_A), URIRef(PRED_1), URIRef("http://example.org/obj")))
        ctx2 = ds.graph(URIRef("http://example.org/g2"))
        ctx2.add((URIRef(URI_B), URIRef(PRED_2), Literal("test")))
        graphs = from_rdflib_dataset(ds)
        identifiers = {g.identifier for g in graphs}
        assert "http://example.org/g1" in identifiers
        assert "http://example.org/g2" in identifiers

    def test_roundtrip_litegraph_to_rdflib_and_back(self):
        original = LiteGraph(identifier="http://example.org/graph")
        original.add((URI_A, PRED_1, OBJ_URI))
        original.add((URI_A, PRED_2, OBJ_LIT))
        original.add((URI_A, PRED_3, OBJ_LANG))
        ds = original.to_rdflib_dataset()
        restored = [g for g in from_rdflib_dataset(ds) if g.identifier == "http://example.org/graph"][0]
        assert len(restored) == len(original)
        assert set(restored) == set(original)
