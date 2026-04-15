# SPDX-FileCopyrightText: 2026 Arcangelo Massari <arcangelo.massari@unibo.it>
#
# SPDX-License-Identifier: ISC

from __future__ import annotations

from typing import NamedTuple

_XSD_STRING = "http://www.w3.org/2001/XMLSchema#string"


class RDFTerm(NamedTuple):
    type: str
    value: str
    datatype: str = ""
    lang: str = ""


Triple = tuple[str, str, RDFTerm]
SPOIndex = dict[str, dict[str, set[RDFTerm]]]
POSIndex = dict[str, dict[RDFTerm, set[str]]]
_InternalSPO = dict[int, dict[int, set[int]]]
_InternalPOS = dict[int, dict[int, set[int]]]
