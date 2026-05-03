# SPDX-FileCopyrightText: 2026 Arcangelo Massari <arcangelo.massari@unibo.it>
#
# SPDX-License-Identifier: ISC

from __future__ import annotations

from collections.abc import Iterable
from typing import Iterator

from triplelite._core import TripleLite as _CTripleLite
from triplelite._rdflib_bridge import to_rdflib as _to_rdflib
from triplelite._types import RDFTerm, Triple


class SubgraphView:
    __slots__ = ("_parent", "_subject")

    def __init__(self, parent: TripleLite, subject: str) -> None:
        self._parent = parent
        self._subject = subject

    def predicate_objects(self, subject: str | None = None) -> Iterator[tuple[str, RDFTerm]]:
        if subject is not None and subject != self._subject:
            return
        yield from self._parent.predicate_objects(subject=self._subject)

    def __iter__(self) -> Iterator[Triple]:
        yield from self._parent.triples((self._subject, None, None))

    def __len__(self) -> int:
        return sum(1 for _ in self)

    def __eq__(self, other: object) -> bool:
        if isinstance(other, (set, frozenset)):
            return set(self) == other
        return NotImplemented

    def __sub__(self, other: set | frozenset) -> set:
        return set(self) - other

    def __rsub__(self, other: set | frozenset) -> set | frozenset:
        return other - set(self)


class TripleLite(_CTripleLite):
    def __init__(
        self,
        identifier: str | None = None,
        reverse_index_predicates: frozenset[str] | None = None,
    ) -> None:
        super().__init__()
        self.identifier = identifier
        self._str_to_id: dict[str, int] = {}
        self._id_to_str: list[str] = []
        self._term_to_id: dict[RDFTerm, int] = {}
        self._id_to_term: list[RDFTerm] = []
        if reverse_index_predicates is not None:
            self._indexed_preds: frozenset[str] | None = frozenset(reverse_index_predicates)
            self._pos: dict[int, dict[int, set[int]]] | None = {}
        else:
            self._indexed_preds = None
            self._pos = None

    def _intern_str(self, s: str) -> int:
        sid = self._str_to_id.get(s)
        if sid is not None:
            return sid
        sid = len(self._id_to_str)
        self._str_to_id[s] = sid
        self._id_to_str.append(s)
        return sid

    def _intern_term(self, t: RDFTerm) -> int:
        tid = self._term_to_id.get(t)
        if tid is not None:
            return tid
        tid = len(self._id_to_term)
        self._term_to_id[t] = tid
        self._id_to_term.append(t)
        return tid

    def add(self, triple: tuple[str, str, RDFTerm]) -> None:
        subject, predicate, obj = triple
        super().add(triple)
        sid = self._intern_str(subject)
        pid = self._intern_str(predicate)
        oid = self._intern_term(obj)
        if self._pos is not None:
            indexed = self._indexed_preds
            if not indexed or predicate in indexed:
                self._pos.setdefault(pid, {}).setdefault(oid, set()).add(sid)

    def add_many(self, triples: Iterable[tuple[str, str, RDFTerm]]) -> None:
        for t in triples:
            self.add(t)

    def remove(self, triple: tuple[str | None, str | None, RDFTerm | None]) -> None:
        subject, predicate, obj = triple
        if subject is None and predicate is None and obj is None:
            super().remove(triple)
            if self._pos is not None:
                self._pos.clear()
            return
        if self._pos is not None:
            matching = list(self.triples(triple))
            super().remove(triple)
            for s, p, o in matching:
                sid = self._str_to_id.get(s)
                pid = self._str_to_id.get(p)
                oid = self._term_to_id.get(o)
                if sid is None or pid is None or oid is None:
                    continue
                obj_to_subjects = self._pos.get(pid)
                if obj_to_subjects is None:
                    continue
                subjects = obj_to_subjects.get(oid)
                if subjects is not None:
                    subjects.discard(sid)
                    if not subjects:
                        del obj_to_subjects[oid]
                if not obj_to_subjects:
                    del self._pos[pid]
        else:
            super().remove(triple)

    def subjects(self, predicate: str | None = None, object: RDFTerm | None = None) -> Iterator[str]:
        pos = self._pos
        if pos is not None:
            id_to_str = self._id_to_str
            pid = self._str_to_id.get(predicate) if predicate is not None else None
            oid = self._term_to_id.get(object) if object is not None else None
            if predicate is not None and pid is None:
                return
            if object is not None and oid is None:
                return
            if pid is not None and oid is not None:
                for sid in pos.get(pid, {}).get(oid, set()):
                    yield id_to_str[sid]
                return
            if pid is not None:
                for subject_set in pos.get(pid, {}).values():
                    for sid in subject_set:
                        yield id_to_str[sid]
                return
            if oid is not None:
                for obj_to_subjects in pos.values():
                    for sid in obj_to_subjects.get(oid, set()):
                        yield id_to_str[sid]
                return
            seen: set[int] = set()
            for obj_to_subjects in pos.values():
                for subject_set in obj_to_subjects.values():
                    for sid in subject_set:
                        if sid not in seen:
                            seen.add(sid)
                            yield id_to_str[sid]
            return
        yield from super().subjects(predicate=predicate, object=object)

    def subgraph(self, subject: str) -> SubgraphView | None:
        if not self.has_subject(subject):
            return None
        return SubgraphView(self, subject)

    def to_rdflib(self):
        return _to_rdflib(self)
