# SPDX-FileCopyrightText: 2026 Arcangelo Massari <arcangelo.massari@unibo.it>
#
# SPDX-License-Identifier: ISC

from __future__ import annotations

from typing import Iterator

from triplelite._rdflib_bridge import to_rdflib as _to_rdflib
from triplelite._types import POSIndex, RDFTerm, SPOIndex, Triple


class TripleLite:
    __slots__ = ("_spo", "_pos", "_indexed_predicates", "_len", "identifier")

    def __init__(
        self,
        identifier: str | None = None,
        reverse_index_predicates: frozenset[str] | None = None,
    ) -> None:
        self._spo: SPOIndex = {}
        self._len: int = 0
        self.identifier: str | None = identifier
        self._indexed_predicates: frozenset[str] | None = reverse_index_predicates
        self._pos: POSIndex | None = {} if reverse_index_predicates is not None else None

    def add(self, triple: tuple[str, str, RDFTerm]) -> None:
        subject, predicate, obj = triple
        objects = self._spo.setdefault(subject, {}).setdefault(predicate, set())
        if obj not in objects:
            objects.add(obj)
            self._len += 1
            if self._pos is not None:
                indexed = self._indexed_predicates
                if not indexed or predicate in indexed:
                    self._pos.setdefault(predicate, {}).setdefault(obj, set()).add(subject)

    def _remove_triple(self, subject: str, predicate: str, obj: RDFTerm) -> None:
        predicates = self._spo.get(subject)
        if predicates is None:
            return
        objects = predicates.get(predicate)
        if objects is None:
            return
        objects.discard(obj)
        self._len -= 1
        if not objects:
            del predicates[predicate]
        if not predicates:
            del self._spo[subject]
        if self._pos is not None:
            obj_to_subjects = self._pos.get(predicate)
            if obj_to_subjects is not None:
                subjects = obj_to_subjects.get(obj)
                if subjects is not None:
                    subjects.discard(subject)
                    if not subjects:
                        del obj_to_subjects[obj]
                if not obj_to_subjects:
                    del self._pos[predicate]

    def remove(self, triple: tuple[str | None, str | None, RDFTerm | None]) -> None:
        subject, predicate, obj = triple
        if subject is None and predicate is None and obj is None:
            self._spo.clear()
            self._len = 0
            if self._pos is not None:
                self._pos.clear()
            return
        for s, p, o in list(self.triples((subject, predicate, obj))):
            self._remove_triple(s, p, o)

    def triples(self, pattern: tuple[str | None, str | None, RDFTerm | None]) -> Iterator[Triple]:
        subject, predicate, obj = pattern
        if subject is not None:
            predicates = self._spo.get(subject)
            if predicates is None:
                return
            if predicate is not None:
                objects = predicates.get(predicate)
                if objects is None:
                    return
                if obj is not None:
                    if obj in objects:
                        yield subject, predicate, obj
                else:
                    for o in objects:
                        yield subject, predicate, o
            else:
                for pred, objects in predicates.items():
                    for o in objects:
                        if obj is None or o == obj:
                            yield subject, pred, o
        else:
            for subj, predicates in self._spo.items():
                for pred, objects in predicates.items():
                    if predicate is not None and pred != predicate:
                        continue
                    for o in objects:
                        if obj is None or o == obj:
                            yield subj, pred, o

    def objects(self, subject: str | None = None, predicate: str | None = None) -> Iterator[RDFTerm]:
        if subject is not None and predicate is not None:
            yield from self._spo.get(subject, {}).get(predicate, set())
            return
        if subject is not None:
            for objects in self._spo.get(subject, {}).values():
                yield from objects
            return
        for predicates in self._spo.values():
            if predicate is not None:
                yield from predicates.get(predicate, set())
            else:
                for objects in predicates.values():
                    yield from objects

    def predicate_objects(self, subject: str | None = None) -> Iterator[tuple[str, RDFTerm]]:
        if subject is not None:
            for pred, objects in self._spo.get(subject, {}).items():
                for obj in objects:
                    yield pred, obj
            return
        for predicates in self._spo.values():
            for pred, objects in predicates.items():
                for obj in objects:
                    yield pred, obj

    def subjects(self, predicate: str | None = None, object: RDFTerm | None = None) -> Iterator[str]:
        pos = self._pos
        if pos is not None:
            if predicate is not None and object is not None:
                yield from pos.get(predicate, {}).get(object, set())
                return
            if predicate is not None:
                for subject_set in pos.get(predicate, {}).values():
                    yield from subject_set
                return
            if object is not None:
                for obj_to_subjects in pos.values():
                    yield from obj_to_subjects.get(object, set())
                return
            seen: set[str] = set()
            for obj_to_subjects in pos.values():
                for subject_set in obj_to_subjects.values():
                    for subject in subject_set:
                        if subject not in seen:
                            seen.add(subject)
                            yield subject
            return

        if predicate is None and object is None:
            yield from self._spo
            return
        for subject, predicates in self._spo.items():
            if predicate is not None:
                objects = predicates.get(predicate)
                if objects is not None and (object is None or object in objects):
                    yield subject
            else:
                for objects in predicates.values():
                    if object in objects:
                        yield subject
                        break

    def subgraph(self, subject: str) -> TripleLite | None:
        predicates = self._spo.get(subject)
        if predicates is None:
            return None
        graph = TripleLite()
        for predicate, objects in predicates.items():
            copied = set(objects)
            graph._spo.setdefault(subject, {})[predicate] = copied
            graph._len += len(copied)
        return graph

    def __contains__(self, triple: tuple[str, str, RDFTerm]) -> bool:
        subject, predicate, obj = triple
        return obj in self._spo.get(subject, {}).get(predicate, set())

    def __iter__(self) -> Iterator[Triple]:
        for subject, predicates in self._spo.items():
            for predicate, objects in predicates.items():
                for obj in objects:
                    yield subject, predicate, obj

    def __len__(self) -> int:
        return self._len

    def to_rdflib(self):
        return _to_rdflib(self)
