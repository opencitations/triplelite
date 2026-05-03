# SPDX-FileCopyrightText: 2026 Arcangelo Massari <arcangelo.massari@unibo.it>
#
# SPDX-License-Identifier: ISC

from rdflib import XSD, Dataset, Graph, Literal, URIRef

from triplelite import RDFTerm, SubgraphView, TripleLite, from_rdflib, rdflib_to_rdfterm, to_rdflib

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
        g = TripleLite()
        g.add((URI_A, PRED_1, OBJ_URI))
        assert len(g) == 1

    def test_add_duplicate_ignored(self):
        g = TripleLite()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_A, PRED_1, OBJ_URI))
        assert len(g) == 1

    def test_add_different_objects_same_sp(self):
        g = TripleLite()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_A, PRED_1, OBJ_LIT))
        assert len(g) == 2

    def test_add_updates_reverse_index(self):
        g = TripleLite(reverse_index_predicates=frozenset({PRED_1}))
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_B, PRED_1, OBJ_URI))
        assert set(g.subjects(PRED_1, OBJ_URI)) == {URI_A, URI_B}

    def test_add_selective_reverse_index(self):
        g = TripleLite(reverse_index_predicates=frozenset({PRED_1}))
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_A, PRED_2, OBJ_LIT))
        assert set(g.subjects(PRED_1, OBJ_URI)) == {URI_A}
        assert list(g.subjects(PRED_2, OBJ_LIT)) == []

    def test_add_index_all_predicates(self):
        g = TripleLite(reverse_index_predicates=frozenset())
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_A, PRED_2, OBJ_LIT))
        assert set(g.subjects(PRED_1, OBJ_URI)) == {URI_A}
        assert set(g.subjects(PRED_2, OBJ_LIT)) == {URI_A}

    def test_no_reverse_index_by_default(self):
        g = TripleLite()
        g.add((URI_A, PRED_1, OBJ_URI))
        assert set(g.subjects(PRED_1, OBJ_URI)) == {URI_A}


class TestAddMany:
    def test_add_many_equivalent_to_add_loop(self):
        triples = [
            (URI_A, PRED_1, OBJ_URI),
            (URI_A, PRED_2, OBJ_LIT),
            (URI_B, PRED_1, OBJ_LANG),
        ]
        g1 = TripleLite()
        for t in triples:
            g1.add(t)
        g2 = TripleLite()
        g2.add_many(triples)
        assert set(g1) == set(g2)
        assert len(g1) == len(g2)

    def test_add_many_empty(self):
        g = TripleLite()
        g.add_many([])
        assert len(g) == 0

    def test_add_many_with_duplicates(self):
        g = TripleLite()
        g.add_many([(URI_A, PRED_1, OBJ_URI), (URI_A, PRED_1, OBJ_URI)])
        assert len(g) == 1

    def test_add_many_with_reverse_index(self):
        g = TripleLite(reverse_index_predicates=frozenset({PRED_1}))
        g.add_many([
            (URI_A, PRED_1, OBJ_URI),
            (URI_B, PRED_1, OBJ_URI),
            (URI_A, PRED_2, OBJ_LIT),
        ])
        assert set(g.subjects(PRED_1, OBJ_URI)) == {URI_A, URI_B}
        assert len(g) == 3


class TestRemove:
    def test_remove_exact_triple(self):
        g = TripleLite()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.remove((URI_A, PRED_1, OBJ_URI))
        assert len(g) == 0

    def test_remove_by_sp_wildcard(self):
        g = TripleLite()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_A, PRED_1, OBJ_LIT))
        g.remove((URI_A, PRED_1, None))
        assert len(g) == 0

    def test_remove_by_subject_wildcard(self):
        g = TripleLite()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_A, PRED_2, OBJ_LIT))
        g.remove((URI_A, None, None))
        assert len(g) == 0

    def test_remove_by_predicate_wildcard(self):
        g = TripleLite()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_B, PRED_1, OBJ_LIT))
        g.remove((None, PRED_1, None))
        assert len(g) == 0

    def test_remove_by_object_wildcard(self):
        g = TripleLite()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_B, PRED_2, OBJ_URI))
        g.remove((None, None, OBJ_URI))
        assert len(g) == 0

    def test_remove_all(self):
        g = TripleLite()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_B, PRED_2, OBJ_LIT))
        g.remove((None, None, None))
        assert len(g) == 0

    def test_remove_cleans_empty_dicts(self):
        g = TripleLite()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.remove((URI_A, PRED_1, OBJ_URI))
        assert not g.has_subject(URI_A)

    def test_remove_updates_reverse_index(self):
        g = TripleLite(reverse_index_predicates=frozenset({PRED_1}))
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_B, PRED_1, OBJ_URI))
        g.remove((URI_A, PRED_1, OBJ_URI))
        assert set(g.subjects(PRED_1, OBJ_URI)) == {URI_B}

    def test_remove_all_clears_reverse_index(self):
        g = TripleLite(reverse_index_predicates=frozenset({PRED_1}))
        g.add((URI_A, PRED_1, OBJ_URI))
        g.remove((None, None, None))
        assert list(g.subjects(PRED_1, OBJ_URI)) == []

    def test_remove_nonexistent_noop(self):
        g = TripleLite()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.remove((URI_B, PRED_1, OBJ_URI))
        assert len(g) == 1

    def test_remove_interned_but_absent_triple(self):
        g = TripleLite()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_A, PRED_2, OBJ_LIT))
        g.remove((URI_A, PRED_1, OBJ_LIT))
        assert len(g) == 2


class TestTriples:
    def test_exact_match(self):
        g = TripleLite()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_A, PRED_1, OBJ_LIT))
        result = list(g.triples((URI_A, PRED_1, OBJ_URI)))
        assert result == [(URI_A, PRED_1, OBJ_URI)]

    def test_sp_pattern(self):
        g = TripleLite()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_A, PRED_1, OBJ_LIT))
        result = list(g.triples((URI_A, PRED_1, None)))
        assert len(result) == 2

    def test_s_pattern(self):
        g = TripleLite()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_A, PRED_2, OBJ_LIT))
        result = list(g.triples((URI_A, None, None)))
        assert len(result) == 2

    def test_p_pattern(self):
        g = TripleLite()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_B, PRED_1, OBJ_LIT))
        g.add((URI_A, PRED_2, OBJ_URI))
        result = list(g.triples((None, PRED_1, None)))
        assert len(result) == 2

    def test_o_pattern(self):
        g = TripleLite()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_B, PRED_2, OBJ_URI))
        g.add((URI_A, PRED_1, OBJ_LIT))
        result = list(g.triples((None, None, OBJ_URI)))
        assert len(result) == 2

    def test_all_wildcard(self):
        g = TripleLite()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_B, PRED_2, OBJ_LIT))
        result = list(g.triples((None, None, None)))
        assert len(result) == 2

    def test_no_match_returns_empty(self):
        g = TripleLite()
        g.add((URI_A, PRED_1, OBJ_URI))
        result = list(g.triples((URI_B, PRED_1, None)))
        assert result == []

    def test_no_match_predicate(self):
        g = TripleLite()
        g.add((URI_A, PRED_1, OBJ_URI))
        result = list(g.triples((URI_A, PRED_2, None)))
        assert result == []


class TestObjects:
    def test_sp_lookup(self):
        g = TripleLite()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_A, PRED_1, OBJ_LIT))
        result = set(g.objects(URI_A, PRED_1))
        assert result == {OBJ_URI, OBJ_LIT}

    def test_subject_only(self):
        g = TripleLite()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_A, PRED_2, OBJ_LIT))
        result = set(g.objects(subject=URI_A))
        assert result == {OBJ_URI, OBJ_LIT}

    def test_predicate_only(self):
        g = TripleLite()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_B, PRED_1, OBJ_LIT))
        result = set(g.objects(predicate=PRED_1))
        assert result == {OBJ_URI, OBJ_LIT}

    def test_no_filter(self):
        g = TripleLite()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_B, PRED_2, OBJ_LIT))
        result = set(g.objects())
        assert result == {OBJ_URI, OBJ_LIT}

    def test_empty_result(self):
        g = TripleLite()
        assert list(g.objects(URI_A, PRED_1)) == []


class TestPredicateObjects:
    def test_with_subject(self):
        g = TripleLite()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_A, PRED_2, OBJ_LIT))
        result = set(g.predicate_objects(URI_A))
        assert result == {(PRED_1, OBJ_URI), (PRED_2, OBJ_LIT)}

    def test_without_subject(self):
        g = TripleLite()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_B, PRED_2, OBJ_LIT))
        result = set(g.predicate_objects())
        assert result == {(PRED_1, OBJ_URI), (PRED_2, OBJ_LIT)}


class TestSubjects:
    def test_with_reverse_index_po(self):
        g = TripleLite(reverse_index_predicates=frozenset({PRED_1}))
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_B, PRED_1, OBJ_URI))
        g.add((URI_C, PRED_1, OBJ_LIT))
        assert set(g.subjects(PRED_1, OBJ_URI)) == {URI_A, URI_B}

    def test_with_reverse_index_p_only(self):
        g = TripleLite(reverse_index_predicates=frozenset({PRED_1}))
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_B, PRED_1, OBJ_LIT))
        assert set(g.subjects(predicate=PRED_1)) == {URI_A, URI_B}

    def test_with_reverse_index_o_only(self):
        g = TripleLite(reverse_index_predicates=frozenset({PRED_1, PRED_2}))
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_B, PRED_2, OBJ_URI))
        assert set(g.subjects(object=OBJ_URI)) == {URI_A, URI_B}

    def test_with_reverse_index_all_subjects(self):
        g = TripleLite(reverse_index_predicates=frozenset({PRED_1}))
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_B, PRED_1, OBJ_LIT))
        assert set(g.subjects()) == {URI_A, URI_B}

    def test_without_reverse_index_fallback(self):
        g = TripleLite()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_B, PRED_1, OBJ_URI))
        assert set(g.subjects(PRED_1, OBJ_URI)) == {URI_A, URI_B}

    def test_without_reverse_index_p_only(self):
        g = TripleLite()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_B, PRED_2, OBJ_LIT))
        assert set(g.subjects(predicate=PRED_1)) == {URI_A}

    def test_without_reverse_index_o_only(self):
        g = TripleLite()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_B, PRED_2, OBJ_URI))
        assert set(g.subjects(object=OBJ_URI)) == {URI_A, URI_B}

    def test_without_reverse_index_all(self):
        g = TripleLite()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_B, PRED_2, OBJ_LIT))
        assert set(g.subjects()) == {URI_A, URI_B}

    def test_empty(self):
        g = TripleLite(reverse_index_predicates=frozenset({PRED_1}))
        assert list(g.subjects(PRED_1, OBJ_URI)) == []

    def test_unindexed_predicate_not_in_pos(self):
        g = TripleLite(reverse_index_predicates=frozenset({PRED_1}))
        g.add((URI_A, PRED_2, OBJ_URI))
        assert list(g.subjects(PRED_2, OBJ_URI)) == []


class TestHasSubject:
    def test_present(self):
        g = TripleLite()
        g.add((URI_A, PRED_1, OBJ_URI))
        assert g.has_subject(URI_A)

    def test_absent(self):
        g = TripleLite()
        assert not g.has_subject(URI_A)

    def test_after_remove_all_triples(self):
        g = TripleLite()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.remove((URI_A, PRED_1, OBJ_URI))
        assert not g.has_subject(URI_A)

    def test_multiple_subjects(self):
        g = TripleLite()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_B, PRED_1, OBJ_LIT))
        assert g.has_subject(URI_A)
        assert g.has_subject(URI_B)
        assert not g.has_subject(URI_C)


class TestSubgraph:
    def test_extracts_subject_triples(self):
        g = TripleLite()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_A, PRED_2, OBJ_LIT))
        g.add((URI_B, PRED_1, OBJ_LIT))
        sub = g.subgraph(URI_A)
        assert sub is not None
        assert isinstance(sub, SubgraphView)
        assert len(sub) == 2
        assert set(sub) == {(URI_A, PRED_1, OBJ_URI), (URI_A, PRED_2, OBJ_LIT)}

    def test_returns_none_for_missing(self):
        g = TripleLite()
        assert g.subgraph(URI_A) is None

    def test_view_reflects_parent_changes(self):
        g = TripleLite()
        g.add((URI_A, PRED_1, OBJ_URI))
        sub = g.subgraph(URI_A)
        assert sub is not None
        g.add((URI_A, PRED_2, OBJ_LIT))
        assert len(sub) == 2
        assert (URI_A, PRED_2, OBJ_LIT) in set(sub)

    def test_predicate_objects(self):
        g = TripleLite()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_A, PRED_2, OBJ_LIT))
        sub = g.subgraph(URI_A)
        assert sub is not None
        assert set(sub.predicate_objects(URI_A)) == {(PRED_1, OBJ_URI), (PRED_2, OBJ_LIT)}

    def test_predicate_objects_wrong_subject(self):
        g = TripleLite()
        g.add((URI_A, PRED_1, OBJ_URI))
        sub = g.subgraph(URI_A)
        assert sub is not None
        assert list(sub.predicate_objects(URI_B)) == []

    def test_iter(self):
        g = TripleLite()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_A, PRED_2, OBJ_LIT))
        sub = g.subgraph(URI_A)
        assert sub is not None
        assert set(sub) == {(URI_A, PRED_1, OBJ_URI), (URI_A, PRED_2, OBJ_LIT)}

    def test_set_difference(self):
        g = TripleLite()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_A, PRED_2, OBJ_LIT))
        sub = g.subgraph(URI_A)
        assert sub is not None
        current = {(URI_A, PRED_1, OBJ_URI), (URI_A, PRED_2, OBJ_LIT), (URI_A, PRED_3, OBJ_INT)}
        added = current - sub
        assert added == {(URI_A, PRED_3, OBJ_INT)}
        removed = sub - current
        assert removed == set()

    def test_eq_with_frozenset(self):
        g = TripleLite()
        g.add((URI_A, PRED_1, OBJ_URI))
        sub = g.subgraph(URI_A)
        assert sub is not None
        assert sub == {(URI_A, PRED_1, OBJ_URI)}
        assert sub != {(URI_A, PRED_1, OBJ_LIT)}


class TestContains:
    def test_contains_true(self):
        g = TripleLite()
        g.add((URI_A, PRED_1, OBJ_URI))
        assert (URI_A, PRED_1, OBJ_URI) in g

    def test_contains_false(self):
        g = TripleLite()
        g.add((URI_A, PRED_1, OBJ_URI))
        assert (URI_A, PRED_1, OBJ_LIT) not in g

    def test_contains_missing_subject(self):
        g = TripleLite()
        assert (URI_A, PRED_1, OBJ_URI) not in g


class TestIter:
    def test_iter_all(self):
        g = TripleLite()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_B, PRED_2, OBJ_LIT))
        result = set(g)
        assert result == {(URI_A, PRED_1, OBJ_URI), (URI_B, PRED_2, OBJ_LIT)}


class TestLen:
    def test_empty(self):
        assert len(TripleLite()) == 0

    def test_after_adds(self):
        g = TripleLite()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_A, PRED_2, OBJ_LIT))
        assert len(g) == 2

    def test_after_remove(self):
        g = TripleLite()
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_A, PRED_2, OBJ_LIT))
        g.remove((URI_A, PRED_1, OBJ_URI))
        assert len(g) == 1


class TestIdentifier:
    def test_default_none(self):
        assert TripleLite().identifier is None

    def test_set_identifier(self):
        g = TripleLite(identifier="http://example.org/graph")
        assert g.identifier == "http://example.org/graph"


class TestRdflibCompat:
    def test_to_rdflib_with_identifier(self):
        g = TripleLite(identifier="http://example.org/graph")
        g.add((URI_A, PRED_1, OBJ_URI))
        g.add((URI_A, PRED_2, OBJ_LIT))
        g.add((URI_A, PRED_3, OBJ_LANG))

        ds = g.to_rdflib()
        assert isinstance(ds, Dataset)
        assert len(ds) == 3
        assert (URIRef(URI_A), URIRef(PRED_1), URIRef("http://example.org/obj"), URIRef("http://example.org/graph")) in ds
        assert (URIRef(URI_A), URIRef(PRED_2), Literal("hello", datatype=XSD.string), URIRef("http://example.org/graph")) in ds
        assert (URIRef(URI_A), URIRef(PRED_3), Literal("ciao", lang="it"), URIRef("http://example.org/graph")) in ds

    def test_to_rdflib_no_identifier(self):
        g = TripleLite()
        g.add((URI_A, PRED_1, OBJ_URI))
        result = g.to_rdflib()
        assert isinstance(result, Graph)
        assert not isinstance(result, Dataset)
        assert len(result) == 1
        assert (URIRef(URI_A), URIRef(PRED_1), URIRef("http://example.org/obj")) in result

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

    def test_to_rdflib_integer_literal(self):
        g = TripleLite()
        g.add((URI_A, PRED_1, OBJ_INT))
        result = g.to_rdflib()
        assert isinstance(result, Graph)
        obj = list(result.objects(URIRef(URI_A), URIRef(PRED_1)))[0]
        assert obj == Literal("42", datatype=XSD.integer)

    def test_to_rdflib_function(self):
        g = TripleLite(identifier="http://example.org/graph")
        g.add((URI_A, PRED_1, OBJ_URI))
        ds = to_rdflib(g)
        assert isinstance(ds, Dataset)
        assert len(ds) == 1

    def test_from_rdflib_graph(self):
        rg = Graph(identifier=URIRef("http://example.org/graph"))
        rg.add((URIRef(URI_A), URIRef(PRED_1), URIRef("http://example.org/obj")))
        rg.add((URIRef(URI_A), URIRef(PRED_2), Literal("hello", datatype=XSD.string)))
        graphs = from_rdflib(rg)
        assert len(graphs) == 1
        lg = graphs[0]
        assert lg.identifier == "http://example.org/graph"
        assert len(lg) == 2
        assert (URI_A, PRED_1, RDFTerm("uri", "http://example.org/obj")) in lg
        assert (URI_A, PRED_2, OBJ_LIT) in lg

    def test_from_rdflib_graph_no_identifier(self):
        rg = Graph()
        rg.add((URIRef(URI_A), URIRef(PRED_1), URIRef("http://example.org/obj")))
        graphs = from_rdflib(rg)
        assert len(graphs) == 1
        assert graphs[0].identifier is None
        assert len(graphs[0]) == 1

    def test_from_rdflib_graph_lang_literal(self):
        rg = Graph()
        rg.add((URIRef(URI_A), URIRef(PRED_1), Literal("ciao", lang="it")))
        lg = from_rdflib(rg)[0]
        assert (URI_A, PRED_1, OBJ_LANG) in lg

    def test_from_rdflib_dataset(self):
        ds = Dataset()
        ctx = ds.graph(URIRef("http://example.org/g1"))
        ctx.add((URIRef(URI_A), URIRef(PRED_1), URIRef("http://example.org/obj")))
        ctx2 = ds.graph(URIRef("http://example.org/g2"))
        ctx2.add((URIRef(URI_B), URIRef(PRED_2), Literal("test")))
        graphs = from_rdflib(ds)
        identifiers = {g.identifier for g in graphs}
        assert "http://example.org/g1" in identifiers
        assert "http://example.org/g2" in identifiers

    def test_roundtrip_triplelite_to_rdflib_and_back(self):
        original = TripleLite(identifier="http://example.org/graph")
        original.add((URI_A, PRED_1, OBJ_URI))
        original.add((URI_A, PRED_2, OBJ_LIT))
        original.add((URI_A, PRED_3, OBJ_LANG))
        ds = original.to_rdflib()
        restored = [g for g in from_rdflib(ds) if g.identifier == "http://example.org/graph"][0]
        assert len(restored) == len(original)
        assert set(restored) == set(original)
