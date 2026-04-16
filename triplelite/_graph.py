# SPDX-FileCopyrightText: 2026 Arcangelo Massari <arcangelo.massari@unibo.it>
#
# SPDX-License-Identifier: ISC

from __future__ import annotations

from collections.abc import Iterable
from typing import Iterator

from triplelite._rdflib_bridge import to_rdflib as _to_rdflib
from triplelite._types import RDFTerm, Triple, _InternalPOS, _InternalSPO

_EMPTY_DICT: dict = {}
_EMPTY_SET: set = set()


class SubgraphView:
    __slots__ = ("_parent", "_subject", "_sid", "_predicates")

    def __init__(self, parent: TripleLite, subject: str, sid: int, predicates: dict[int, set[int]]) -> None:
        self._parent = parent
        self._subject = subject
        self._sid = sid
        self._predicates = predicates

    def predicate_objects(self, subject: str | None = None) -> Iterator[tuple[str, RDFTerm]]:
        if subject is not None and subject != self._subject:
            return
        id_to_str = self._parent._id_to_str
        id_to_term = self._parent._id_to_term
        for pid, objects in self._predicates.items():
            p_str = id_to_str[pid]
            for oid in objects:
                yield p_str, id_to_term[oid]

    def __iter__(self) -> Iterator[Triple]:
        subject = self._subject
        id_to_str = self._parent._id_to_str
        id_to_term = self._parent._id_to_term
        for pid, objects in self._predicates.items():
            p_str = id_to_str[pid]
            for oid in objects:
                yield subject, p_str, id_to_term[oid]

    def __len__(self) -> int:
        return sum(len(objects) for objects in self._predicates.values())

    def __eq__(self, other: object) -> bool:
        if isinstance(other, (set, frozenset)):
            return set(self) == other
        return NotImplemented

    def __sub__(self, other: set | frozenset) -> set:
        return set(self) - other

    def __rsub__(self, other: set | frozenset) -> set | frozenset:
        return other - set(self)


class TripleLite:
    __slots__ = (
        "_spo",
        "_pos",
        "_indexed_predicates",
        "_len",
        "identifier",
        "_str_to_id",
        "_id_to_str",
        "_term_to_id",
        "_id_to_term",
    )

    def __init__(
        self,
        identifier: str | None = None,
        reverse_index_predicates: frozenset[str] | None = None,
    ) -> None:
        self._spo: _InternalSPO = {}
        self._len: int = 0
        self.identifier: str | None = identifier
        self._str_to_id: dict[str, int] = {}
        self._id_to_str: list[str] = []
        self._term_to_id: dict[RDFTerm, int] = {}
        self._id_to_term: list[RDFTerm] = []
        if reverse_index_predicates is not None:
            self._indexed_predicates: frozenset[int] | None = frozenset(
                self._intern_str(p) for p in reverse_index_predicates
            )
            self._pos: _InternalPOS | None = {}
        else:
            self._indexed_predicates = None
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
        sid = self._intern_str(subject)
        pid = self._intern_str(predicate)
        oid = self._intern_term(obj)
        objects = self._spo.setdefault(sid, {}).setdefault(pid, set())
        if oid not in objects:
            objects.add(oid)
            self._len += 1
            if self._pos is not None:
                indexed = self._indexed_predicates
                if not indexed or pid in indexed:
                    self._pos.setdefault(pid, {}).setdefault(oid, set()).add(sid)

    def add_many(self, triples: Iterable[tuple[str, str, RDFTerm]]) -> None:
        spo = self._spo
        pos = self._pos
        indexed = self._indexed_predicates
        spo_setdefault = spo.setdefault
        intern_str = self._intern_str
        intern_term = self._intern_term
        count = self._len
        if pos is not None:
            pos_setdefault = pos.setdefault
            check_indexed = bool(indexed)
            for subject, predicate, obj in triples:
                sid = intern_str(subject)
                pid = intern_str(predicate)
                oid = intern_term(obj)
                objects = spo_setdefault(sid, {}).setdefault(pid, set())
                if oid not in objects:
                    objects.add(oid)
                    count += 1
                    if not check_indexed or pid in indexed:
                        pos_setdefault(pid, {}).setdefault(oid, set()).add(sid)
        else:
            for subject, predicate, obj in triples:
                sid = intern_str(subject)
                pid = intern_str(predicate)
                oid = intern_term(obj)
                objects = spo_setdefault(sid, {}).setdefault(pid, set())
                if oid not in objects:
                    objects.add(oid)
                    count += 1
        self._len = count

    def _remove_triple(self, sid: int, pid: int, oid: int) -> None:
        predicates = self._spo.get(sid)
        if predicates is None:
            return
        objects = predicates.get(pid)
        if objects is None:
            return
        if oid not in objects:
            return
        objects.discard(oid)
        self._len -= 1
        if not objects:
            del predicates[pid]
        if not predicates:
            del self._spo[sid]
        if self._pos is not None:
            obj_to_subjects = self._pos.get(pid)
            if obj_to_subjects is not None:
                subjects = obj_to_subjects.get(oid)
                if subjects is not None:
                    subjects.discard(sid)
                    if not subjects:
                        del obj_to_subjects[oid]
                if not obj_to_subjects:
                    del self._pos[pid]

    def remove(self, triple: tuple[str | None, str | None, RDFTerm | None]) -> None:
        subject, predicate, obj = triple
        if subject is None and predicate is None and obj is None:
            self._spo.clear()
            self._len = 0
            if self._pos is not None:
                self._pos.clear()
            return
        sid = self._str_to_id.get(subject) if subject is not None else None
        pid = self._str_to_id.get(predicate) if predicate is not None else None
        oid = self._term_to_id.get(obj) if obj is not None else None
        if subject is not None and sid is None:
            return
        if predicate is not None and pid is None:
            return
        if obj is not None and oid is None:
            return
        if sid is not None and pid is not None and oid is not None:
            self._remove_triple(sid, pid, oid)
            return
        to_remove: list[tuple[int, int, int]] = []
        spo = self._spo
        if sid is not None:
            predicates = spo.get(sid)
            if predicates is None:
                return
            if pid is not None:
                objects = predicates.get(pid)
                if objects is None:
                    return
                to_remove.extend((sid, pid, o) for o in objects)
            else:
                for p, objects in predicates.items():
                    if oid is not None:
                        if oid in objects:
                            to_remove.append((sid, p, oid))
                    else:
                        to_remove.extend((sid, p, o) for o in objects)
        else:
            for s, predicates in spo.items():
                if pid is not None:
                    objects = predicates.get(pid)
                    if objects is None:
                        continue
                    if oid is not None:
                        if oid in objects:
                            to_remove.append((s, pid, oid))
                    else:
                        to_remove.extend((s, pid, o) for o in objects)
                else:
                    for p, objects in predicates.items():
                        if oid is not None:
                            if oid in objects:
                                to_remove.append((s, p, oid))
                        else:
                            to_remove.extend((s, p, o) for o in objects)
        for s, p, o in to_remove:
            self._remove_triple(s, p, o)

    def triples(self, pattern: tuple[str | None, str | None, RDFTerm | None]) -> Iterator[Triple]:
        subject, predicate, obj = pattern
        id_to_str = self._id_to_str
        id_to_term = self._id_to_term
        sid = self._str_to_id.get(subject) if subject is not None else None
        pid = self._str_to_id.get(predicate) if predicate is not None else None
        oid = self._term_to_id.get(obj) if obj is not None else None
        if subject is not None and sid is None:
            return
        if predicate is not None and pid is None:
            return
        if obj is not None and oid is None:
            return
        if sid is not None:
            predicates = self._spo.get(sid)
            if predicates is None:
                return
            s_str = id_to_str[sid]
            if pid is not None:
                objects = predicates.get(pid)
                if objects is None:
                    return
                p_str = id_to_str[pid]
                if oid is not None:
                    if oid in objects:
                        yield s_str, p_str, id_to_term[oid]
                else:
                    for o in objects:
                        yield s_str, p_str, id_to_term[o]
            else:
                for p, objects in predicates.items():
                    p_str = id_to_str[p]
                    for o in objects:
                        if oid is None or o == oid:
                            yield s_str, p_str, id_to_term[o]
        else:
            for s, predicates in self._spo.items():
                s_str = id_to_str[s]
                for p, objects in predicates.items():
                    if pid is not None and p != pid:
                        continue
                    p_str = id_to_str[p]
                    for o in objects:
                        if oid is None or o == oid:
                            yield s_str, p_str, id_to_term[o]

    def objects(self, subject: str | None = None, predicate: str | None = None) -> Iterator[RDFTerm]:
        id_to_term = self._id_to_term
        if subject is not None and predicate is not None:
            sid = self._str_to_id.get(subject)
            if sid is None:
                return
            pid = self._str_to_id.get(predicate)
            if pid is None:
                return
            for oid in self._spo.get(sid, _EMPTY_DICT).get(pid, _EMPTY_SET):
                yield id_to_term[oid]
            return
        if subject is not None:
            sid = self._str_to_id.get(subject)
            if sid is None:
                return
            for objects in self._spo.get(sid, _EMPTY_DICT).values():
                for oid in objects:
                    yield id_to_term[oid]
            return
        if predicate is not None:
            pid = self._str_to_id.get(predicate)
            if pid is None:
                return
            for predicates in self._spo.values():
                for oid in predicates.get(pid, _EMPTY_SET):
                    yield id_to_term[oid]
        else:
            for predicates in self._spo.values():
                for objects in predicates.values():
                    for oid in objects:
                        yield id_to_term[oid]

    def predicate_objects(self, subject: str | None = None) -> Iterator[tuple[str, RDFTerm]]:
        id_to_str = self._id_to_str
        id_to_term = self._id_to_term
        if subject is not None:
            sid = self._str_to_id.get(subject)
            if sid is None:
                return
            for pid, objects in self._spo.get(sid, _EMPTY_DICT).items():
                p_str = id_to_str[pid]
                for oid in objects:
                    yield p_str, id_to_term[oid]
            return
        for predicates in self._spo.values():
            for pid, objects in predicates.items():
                p_str = id_to_str[pid]
                for oid in objects:
                    yield p_str, id_to_term[oid]

    def subjects(self, predicate: str | None = None, object: RDFTerm | None = None) -> Iterator[str]:
        id_to_str = self._id_to_str
        pos = self._pos
        if pos is not None:
            pid = self._str_to_id.get(predicate) if predicate is not None else None
            oid = self._term_to_id.get(object) if object is not None else None
            if predicate is not None and pid is None:
                return
            if object is not None and oid is None:
                return
            if pid is not None and oid is not None:
                for sid in pos.get(pid, _EMPTY_DICT).get(oid, _EMPTY_SET):
                    yield id_to_str[sid]
                return
            if pid is not None:
                for subject_set in pos.get(pid, _EMPTY_DICT).values():
                    for sid in subject_set:
                        yield id_to_str[sid]
                return
            if oid is not None:
                for obj_to_subjects in pos.values():
                    for sid in obj_to_subjects.get(oid, _EMPTY_SET):
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

        if predicate is None and object is None:
            for sid in self._spo:
                yield id_to_str[sid]
            return
        pid = self._str_to_id.get(predicate) if predicate is not None else None
        oid = self._term_to_id.get(object) if object is not None else None
        if predicate is not None and pid is None:
            return
        if object is not None and oid is None:
            return
        for sid, predicates in self._spo.items():
            if pid is not None:
                objects = predicates.get(pid)
                if objects is not None and (oid is None or oid in objects):
                    yield id_to_str[sid]
            else:
                for objects in predicates.values():
                    if oid in objects:
                        yield id_to_str[sid]
                        break

    def has_subject(self, subject: str) -> bool:
        sid = self._str_to_id.get(subject)
        return sid is not None and sid in self._spo

    def subgraph(self, subject: str) -> SubgraphView | None:
        sid = self._str_to_id.get(subject)
        if sid is None:
            return None
        predicates = self._spo.get(sid)
        if predicates is None:
            return None
        return SubgraphView(self, subject, sid, predicates)

    def __contains__(self, triple: tuple[str, str, RDFTerm]) -> bool:
        subject, predicate, obj = triple
        sid = self._str_to_id.get(subject)
        if sid is None:
            return False
        pid = self._str_to_id.get(predicate)
        if pid is None:
            return False
        oid = self._term_to_id.get(obj)
        if oid is None:
            return False
        predicates = self._spo.get(sid)
        if predicates is None:
            return False
        objects = predicates.get(pid)
        if objects is None:
            return False
        return oid in objects

    def __iter__(self) -> Iterator[Triple]:
        id_to_str = self._id_to_str
        id_to_term = self._id_to_term
        for sid, predicates in self._spo.items():
            s_str = id_to_str[sid]
            for pid, objects in predicates.items():
                p_str = id_to_str[pid]
                for oid in objects:
                    yield s_str, p_str, id_to_term[oid]

    def __len__(self) -> int:
        return self._len

    def to_rdflib(self):
        return _to_rdflib(self)
