# SPDX-FileCopyrightText: 2026 Arcangelo Massari <arcangelo.massari@unibo.it>
#
# SPDX-License-Identifier: ISC

from hypothesis import given, settings
from hypothesis import strategies as st

from triplelite import RDFTerm, TripleLite

uris = st.from_regex(r"http://example\.org/[a-z]{1,5}", fullmatch=True)
predicates = st.from_regex(r"http://example\.org/p[1-5]", fullmatch=True)

rdf_terms = st.one_of(
    uris.map(lambda u: RDFTerm("uri", u)),
    st.tuples(
        st.text(min_size=1, max_size=10, alphabet="abcdefghij"),
        st.sampled_from(["http://www.w3.org/2001/XMLSchema#string", "http://www.w3.org/2001/XMLSchema#integer"]),
    ).map(lambda t: RDFTerm("literal", t[0], t[1])),
)

triples = st.tuples(uris, predicates, rdf_terms)


class TestSPOPOSConsistency:
    @given(st.lists(triples, max_size=50))
    @settings(max_examples=200)
    def test_add_keeps_indices_in_sync(self, triple_list):
        g = TripleLite(reverse_index_predicates=frozenset())
        for t in triple_list:
            g.add(t)
        for s, p, o in g:
            assert s in set(g.subjects(p, o))

    @given(st.lists(triples, min_size=1, max_size=30), st.data())
    @settings(max_examples=100)
    def test_add_then_remove_keeps_sync(self, triple_list, data):
        g = TripleLite(reverse_index_predicates=frozenset())
        for t in triple_list:
            g.add(t)
        to_remove = data.draw(st.sampled_from(triple_list))
        g.remove(to_remove)
        for s, p, o in g:
            assert s in set(g.subjects(p, o))

    @given(st.lists(triples, max_size=50))
    @settings(max_examples=200)
    def test_add_many_matches_sequential_add(self, triple_list):
        g1 = TripleLite(reverse_index_predicates=frozenset())
        for t in triple_list:
            g1.add(t)
        g2 = TripleLite(reverse_index_predicates=frozenset())
        g2.add_many(triple_list)
        assert set(g1) == set(g2)
        assert len(g1) == len(g2)
        for s, p, o in g2:
            assert s in set(g2.subjects(p, o))

    @given(st.lists(triples, max_size=30))
    @settings(max_examples=100)
    def test_len_matches_iteration_count(self, triple_list):
        g = TripleLite()
        for t in triple_list:
            g.add(t)
        assert len(g) == len(list(g))

    @given(st.lists(triples, max_size=30))
    @settings(max_examples=100)
    def test_selective_index_subset_of_full(self, triple_list):
        preds = set()
        for _, p, _ in triple_list:
            preds.add(p)
        if not preds:
            return
        half = frozenset(list(preds)[:len(preds) // 2 + 1])
        g_full = TripleLite(reverse_index_predicates=frozenset())
        g_sel = TripleLite(reverse_index_predicates=half)
        for t in triple_list:
            g_full.add(t)
            g_sel.add(t)
        for p in half:
            for s, _, o in g_full.triples((None, p, None)):
                full_subjects = set(g_full.subjects(p, o))
                sel_subjects = set(g_sel.subjects(p, o))
                assert full_subjects == sel_subjects

    @given(st.lists(triples, min_size=1, max_size=30))
    @settings(max_examples=100)
    def test_subgraph_contains_all_subject_triples(self, triple_list):
        g = TripleLite()
        for t in triple_list:
            g.add(t)
        subject = triple_list[0][0]
        sub = g.subgraph(subject)
        if sub is None:
            assert not g.has_subject(subject)
            return
        expected = set(g.triples((subject, None, None)))
        actual = set(sub)
        assert expected == actual
