# SPDX-FileCopyrightText: 2026 Arcangelo Massari <arcangelo.massari@unibo.it>
#
# SPDX-License-Identifier: ISC

from __future__ import annotations

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
    def subgraph(self, subject: str) -> SubgraphView | None:
        if not self.has_subject(subject):
            return None
        return SubgraphView(self, subject)

    def to_rdflib(self):
        return _to_rdflib(self)
