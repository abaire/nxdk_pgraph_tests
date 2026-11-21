#!/usr/bin/env python3
"""Generates the vertex shader exceptional value test suites from a hardware log.

Usage: generate_vsh_exceptional_cases.py [--allow-dirty] vsh_log.json

vsh_log.json is the log written by the VSH test XBE from nxdk_low_level_nv2a_tests when it runs on a real Xbox. Each
result event of its "VSH Exceptional *" tests carries the operands and the measured result. The script writes
src/tests/vertex_shader_exceptional_value_tests_generated.cpp, which holds the result tables, the test registration
and the test documentation. Logs from test builds with uncommitted changes are rejected unless --allow-dirty is given.
"""

from __future__ import annotations

import argparse
import json
import re
import struct
import sys
import textwrap
from collections import OrderedDict
from pathlib import Path

ROWS_PER_TEST = 14
MAX_LABEL = 20

NAMES = {
    0x00000000: "0",
    0x80000000: "-0",
    0x7F800000: "inf",
    0xFF800000: "-inf",
    0x7FFFFFFF: "NaN",
    0xFFFFFFFF: "-NaN",
    0x7FC00000: "qNaN",
    0xFFC00000: "-qNaN",
    0x7FA5A5A5: "sNaN",
    0x7FE5A5A5: "sNaNq",
    0x7F800001: "NaN1",
    0x7F7FFFFF: "MAX",
    0xFF7FFFFF: "-MAX",
    0x00800000: "2^-126",
    0x80800000: "-2^-126",
    0x00400000: "den",
    0x80400000: "-den",
    0x5F800000: "2^64",
    0xDF800000: "-2^64",
    0x1F800000: "2^-64",
    0x9F800000: "-2^-64",
}


# Biased exponent of inf and NaN.
_EXPONENT_SPECIAL = 0xFF
# Powers of two in this exponent range are labeled as decimals, others as 2^e.
_MIN_DECIMAL_EXPONENT = -4
_MAX_DECIMAL_EXPONENT = 8
# Longest decimal label before falling back to hex.
_MAX_DECIMAL_LABEL = 7


def tok(bits: int) -> str:
    if bits in NAMES:
        return NAMES[bits]
    exp = (bits >> 23) & 0xFF
    man = bits & 0x7FFFFF
    sign = "-" if bits >> 31 else ""
    if exp == _EXPONENT_SPECIAL:
        return f"{sign}NaN?" if man else f"{sign}inf"
    if exp == 0:
        return f"0x{bits:08X}"
    if man == 0:
        e = exp - 127
        if _MIN_DECIMAL_EXPONENT <= e <= _MAX_DECIMAL_EXPONENT:
            return f"{sign}{2.0**e:g}"
        return f"{sign}2^{e}"
    f = struct.unpack(">f", struct.pack(">I", bits))[0]
    for precision in range(4, 8):
        s = f"{f:.{precision}g}"
        if len(s) > _MAX_DECIMAL_LABEL:
            break
        if struct.pack(">f", float(s)) == struct.pack(">f", f):
            return s
    return f"0x{bits:08X}"


def h(s: str) -> int:
    return int(s, 16)


def vec(v) -> list[int]:
    if isinstance(v, list):
        return [h(x) for x in v]
    x = h(v)
    return [x, x, x, x]


def short_case(case: str) -> str:
    s = case
    for k, v in (
        ("0x7FFFFFFF", "NaN"),
        ("0xFFFFFFFF", "-NaN"),
        ("0x7FC00000", "qNaN"),
        ("0xFFC00000", "-qNaN"),
        ("0x7FA5A5A5", "sNaN"),
        ("-denormal", "-den"),
        ("denormal", "den"),
        ("-FLT_MAX", "-MAX"),
        ("FLT_MAX", "MAX"),
    ):
        s = s.replace(k, v)
    return s.replace(" * ", "*").replace(" + ", "+").replace(" - ", "-").replace(", ", ",")


def vec_label(v: list[int]) -> str:
    return ",".join(tok(x) for x in v)


COMPONENTS = "xyzw"

# (group, op) -> (test name prefix, shader, lane rule)
#   shader: name of the shader array in vertex_shader_exceptional_value_tests.cpp
#   lane rule: "c" (component of the record) or an explicit lane character.
OPS = OrderedDict(
    [
        (("mac", "MUL"), ("MUL", "kMacMulShader", "x")),
        (("mac", "ADD"), ("ADD", "kMacAddShader", "x")),
        (("mac", "MAD"), ("MAD", "kMacMadShader", "x")),
        (("mac", "MIN"), ("MIN", "kMacMinShader", "x")),
        (("mac", "MAX"), ("MAX", "kMacMaxShader", "x")),
        (("mac", "SLT"), ("SLT", "kMacSltShader", "x")),
        (("mac", "SGE"), ("SGE", "kMacSgeShader", "x")),
        (("mac", "DP3"), ("DP3", "kMacDotShader", "x")),
        (("mac", "DPH"), ("DPH", "kMacDotShader", "y")),
        (("mac", "DP4"), ("DP4", "kMacDotShader", "z")),
        (("mac", "DST"), ("DST", "kMacDstShader", "c")),
        (("mov", "MOV c"), ("MOV_c", "kMovCShader", "c")),
        (("mov", "MOV -c"), ("MOV_neg_c", "kMovNegCShader", "c")),
        (("mov", "MOV c.wzyx"), ("MOV_c_wzyx", "kMovCWzyxShader", "c")),
        (("mov", "MOV -c.yxwz"), ("MOV_neg_c_yxwz", "kMovNegCYxwzShader", "c")),
        (("mov", "MOV r0"), ("MOV_r0", "kMovR0Shader", "c")),
        (("mov", "MOV -r0"), ("MOV_neg_r0", "kMovNegR0Shader", "c")),
        (("mov", "MOV v1"), ("MOV_v1", "kMovV1Shader", "c")),
        (("mov", "MOV -v1"), ("MOV_neg_v1", "kMovNegV1Shader", "c")),
        (("ilu", "RCP"), ("RCP", "kIluRcpShader", "x")),
        (("ilu", "RCC"), ("RCC", "kIluRccShader", "x")),
        (("ilu", "RSQ"), ("RSQ", "kIluRsqShader", "x")),
        (("ilu", "EXPP"), ("EXPP", "kIluExppShader", "c")),
        (("ilu", "LOGP"), ("LOGP", "kIluLogpShader", "c")),
        (("ilu", "LIT"), ("LIT", "kIluLitShader", "c")),
    ]
)

# op -> (test name prefix, shader suffix, lane rule). The shader is kPaths<Attr|Const><suffix>Shader.
PATH_OPS = OrderedDict(
    [
        ("A * B", ("MUL_AB", "MulAb", "c")),
        ("B * A", ("MUL_BA", "MulBa", "c")),
        ("A * B + C", ("MAD_AB", "MadAb", "c")),
        ("B * A + C", ("MAD_BA", "MadBa", "c")),
        ("DP4(A, B)", ("Dot", "Dot", "x")),
        ("DP3(A, B)", ("Dot", "Dot", "y")),
        ("DPH(A, B)", ("Dot", "Dot", "z")),
        ("DPH(B, A)", ("Dot", "Dot", "w")),
        ("A + B", ("ADD_AB", "Add", "c")),
        ("MIN(A, B)", ("MIN_AB", "Min", "c")),
        ("MAX(A, B)", ("MAX_AB", "Max", "c")),
    ]
)

FOG_OPS = OrderedDict(
    [
        ("rcp(P.w)", ("Scalars", "x", "rcp")),
        ("d2", ("Scalars", "y", "d2")),
        ("rsq(d2)", ("Scalars", "z", "rsq")),
        ("d2 * rsq(d2)", ("Scalars", "w", "d")),
        ("P * rcp(P.w)", ("P", "c", "p")),
    ]
)


_REPO = Path(__file__).resolve().parent.parent
_OUT_PATH = _REPO / "src" / "tests" / "vertex_shader_exceptional_value_tests_generated.cpp"
_TEST_PREFIX = "VSH Exceptional "
_OPERANDS = (("in_a", "a"), ("in_b", "b"), ("in_c", "c"), ("in_P", "P"))


def _operand(value: str) -> list[str] | str:
    bits = value.split(",")
    return bits if len(bits) > 1 else bits[0]


def load_records(log, *, allow_dirty: bool):
    """Returns the source revision and the exceptional value result records of a hardware log."""
    revision = log.get("git_revision")
    dirty = log.get("git_dirty")
    if not isinstance(revision, str) or not isinstance(dirty, bool):
        sys.exit("The log does not record the git revision of the test build")
    if not allow_dirty and (dirty or revision == "unknown"):
        sys.exit(f"The test build ({revision}) had uncommitted changes or an unknown revision")

    records = []
    for test in log["tests"]:
        if not test["name"].startswith(_TEST_PREFIX):
            continue
        for event in test["events"]:
            fields = event.get("fields", {})
            if "record_op" not in fields:
                continue
            record = {
                "group": fields["group"],
                "op": fields["record_op"],
                "case": fields["case"],
                "inputs": {name: _operand(fields[key]) for key, name in _OPERANDS if key in fields},
            }
            if "component" in fields:
                record["component"] = fields["component"]
            if fields["record_op"] == "ARL":
                # "kind" is "zero" when c[A0] read all zeros, otherwise "constant" with the marker index read.
                record["arl_read"] = None if fields["kind"] == "zero" else int(fields["decoded_index"])
            else:
                record["hw"] = fields["actual"]
            records.append(record)
    if not records:
        sys.exit(f'The log has no "{_TEST_PREFIX}*" result events')
    return revision + (" (dirty)" if dirty else ""), records


class Record:
    def __init__(self, label, a, b, c, lane, hw, description):
        self.label = label
        self.a = a
        self.b = b
        self.c = c
        self.lane = lane
        self.hw = hw
        self.description = description


def make_label(base: str, comp: str | None) -> str:
    suffix = f".{comp}" if comp else ""
    label = base
    if len(label) + len(suffix) > MAX_LABEL:
        label = label[: MAX_LABEL - len(suffix)]
    return label + suffix


def describe(label: str, hw: int, case: str | None = None) -> str:
    desc = f"{label}"
    if case and short_case(case) != label.rsplit(".", 1)[0]:
        desc += f" [{case}]"
    desc += f": hw {tok(hw)}"
    return desc


def build(data):
    by_key = OrderedDict()
    for r in data:
        by_key.setdefault((r["group"], r["op"]), []).append(r)

    # Each suite is a list of (test_name, shader, [records], title)
    suites = OrderedDict((("mac", []), ("mov", []), ("ilu", []), ("paths", [])))

    def paginate(suite, name, shader, title, records):
        num_tests = (len(records) + ROWS_PER_TEST - 1) // ROWS_PER_TEST
        per_test = (len(records) + num_tests - 1) // num_tests
        tests = [records[i : i + per_test] for i in range(0, len(records), per_test)]
        for i, test_case in enumerate(tests):
            test_name = name if len(tests) == 1 else f"{name}_{i + 1}"
            suites[suite].append((test_name, shader, test_case, title))

    for key, (name, shader, lane_rule) in OPS.items():
        recs = []
        for r in by_key[key]:
            inp = r["inputs"]
            comp = r.get("component")
            lane = comp if lane_rule == "c" else lane_rule
            a = vec(inp["a"])
            b = vec(inp["b"]) if "b" in inp else [0, 0, 0, 0]
            c = vec(inp["c"]) if "c" in inp else [0, 0, 0, 0]
            case_index = list(OrderedDict.fromkeys(x["case"] for x in by_key[key])).index(r["case"]) + 1
            if key[1] == "DST":
                ci = COMPONENTS.index(comp)
                detail = [
                    "1",
                    f"{tok(a[1])}*{tok(b[1])}",
                    f"a.z={tok(a[2])}",
                    f"b.w={tok(b[3])}",
                ][ci]
                base = f"{case_index}:{detail}"
            elif key[1] in ("DP3", "DPH", "DP4"):
                base = f"{case_index}:{vec_label(a)}|{vec_label(b)}"
            elif key[1] == "LIT":
                base = r["case"].replace("N.L ", "L").replace(", N.H ", " H").replace(", power ", " p")
                base = short_case(base)
            elif key[0] == "mov":
                base = r["case"]
            else:
                base = short_case(r["case"])
            if key[1] in ("EXPP", "LOGP", "RCP", "RCC", "RSQ"):
                base = tok(a[0])
            label = make_label(base, comp)
            hw = h(r["hw"])
            recs.append(Record(label, a, b, c, lane, hw, describe(label, hw, r["case"])))
        suite = key[0]
        paginate(suite, name, shader, f"{key[1]} ({len(recs)} values)", recs)

    # ARL
    arl_records = []
    for r in by_key[("ilu", "ARL")]:
        a = vec(r["inputs"]["a"])
        label = make_label(tok(a[0]), None)
        index = r["arl_read"]
        arl_records.append(
            (
                label,
                a,
                index,
                f"{label}: hw reads {'zero' if index is None else f'c[{index}]'}",
            )
        )
    num_tests = (len(arl_records) + ROWS_PER_TEST - 1) // ROWS_PER_TEST
    per_test = (len(arl_records) + num_tests - 1) // num_tests
    arl_tests = [arl_records[i : i + per_test] for i in range(0, len(arl_records), per_test)]
    arl_tests = [("ARL" if len(arl_tests) == 1 else f"ARL_{i + 1}", test_case) for i, test_case in enumerate(arl_tests)]

    # Paths
    for path in ("const", "attr"):
        groups = OrderedDict()
        for op, (name, shader_suffix, lane_rule) in PATH_OPS.items():
            for r in by_key[("paths", f"{path} {op}")]:
                inp = r["inputs"]
                comp = r.get("component")
                lane = comp if lane_rule == "c" else lane_rule
                a = vec(inp["a"])
                b = vec(inp["b"])
                c = vec(inp["c"]) if "c" in inp else [0, 0, 0, 0]
                ci = COMPONENTS.index(comp) if comp else None
                if name == "Dot":
                    case_index = [x["case"] for x in by_key[("paths", f"{path} {op}")]].index(r["case"]) + 1
                    base = f"{op.split('(')[0]}{'' if 'B, A' not in op else 'ba'} case{case_index}"
                elif "+ C" in op:
                    base = (
                        f"{tok(a[ci])}*{tok(b[ci])}+{tok(c[ci])}"
                        if "A * B" in op
                        else f"{tok(b[ci])}*{tok(a[ci])}+{tok(c[ci])}"
                    )
                elif "*" in op:
                    base = f"{tok(a[ci])}*{tok(b[ci])}" if op == "A * B" else f"{tok(b[ci])}*{tok(a[ci])}"
                elif "+" in op:
                    base = f"{tok(a[ci])}+{tok(b[ci])}"
                else:
                    base = f"{op[:3]}({tok(a[ci])},{tok(b[ci])})"
                label = make_label(base, comp)
                hw = h(r["hw"])
                prefix = "Const" if path == "const" else "Attr"
                shader = f"kPaths{prefix}{shader_suffix}Shader"
                groups.setdefault((f"{prefix}_{name}", shader), []).append(
                    Record(
                        label,
                        a,
                        b,
                        c,
                        lane,
                        hw,
                        describe(label, hw, r["case"]),
                    )
                )
        for (name, shader), recs in groups.items():
            paginate(
                "paths",
                name,
                shader,
                f"{path} {name.split('_', 1)[1]} ({len(recs)} values)",
                recs,
            )

    # Fog chain
    for path in ("const", "attr"):
        groups = OrderedDict()
        for op, (name, lane_rule, short) in FOG_OPS.items():
            for r in by_key[("fog_chain", f"{path} {op}")]:
                p = vec(r["inputs"]["P"])
                comp = r.get("component")
                lane = comp if lane_rule == "c" else lane_rule
                base = f"{short} {vec_label(p)}"
                if len(base) + (2 if comp else 0) > MAX_LABEL:
                    base = f"{short} {','.join(tok(x) for x in p).replace(',', '')}"
                label = make_label(base, comp)
                hw = h(r["hw"])
                groups.setdefault(name, []).append(
                    Record(
                        label,
                        p,
                        [0] * 4,
                        [0] * 4,
                        lane,
                        hw,
                        describe(label, hw, f"P = {r['case']}"),
                    )
                )
        prefix = "Const" if path == "const" else "Attr"
        for name, recs in groups.items():
            shader = f"kFog{prefix}{name}Shader"
            paginate(
                "paths",
                f"{prefix}_Fog_{name}",
                shader,
                f"fog chain {path} {name}",
                recs,
            )

    return suites, arl_tests


def c_vec(v):
    return "{" + ", ".join(f"0x{x:08X}" for x in v) + "}"


def ident(suite: str, test_name: str) -> str:
    return "k" + re.sub(r"[^A-Za-z0-9]", "", suite.title()) + re.sub(r"[^A-Za-z0-9]", "", test_name.title())


def shader_file(shader: str) -> str:
    """Maps a shader array name such as kMovNegCYxwzShader to its vsh file stem, mov_neg_c_yxwz."""
    camel = shader[1 : -len("Shader")]
    return re.sub(r"(?<!^)(?=[A-Z])", "_", camel).lower()


SUITES = OrderedDict(
    (
        ("mac", "VertexShaderExceptionalMACTests"),
        ("mov", "VertexShaderExceptionalMOVTests"),
        ("ilu", "VertexShaderExceptionalILUTests"),
        ("paths", "VertexShaderExceptionalPathsTests"),
    )
)

SUITE_INTROS = {
    "mac": "Renders MUL, ADD, MAD, MIN, MAX, SLT, SGE, DP3, DPH, DP4 and DST results on exceptional inputs.",
    "mov": "Renders MOV results, with negation and swizzles, from constants, temporaries and the v1 attribute.",
    "ilu": "Renders RCP, RCC, RSQ, EXPP, LOGP and LIT results, and the constants that ARL addresses, on exceptional "
    "inputs.",
    "paths": "Renders MAC results with A from the v1 attribute (Attr_) or a constant (Const_), and the fog distance chain "
    "rcp(w), P * rcp(w), d2 = DP3, rsq(d2), d2 * rsq(d2) with P from v1 or a constant.",
}

ARL_DOC = (
    "ARL a0, v9 then MOV oD0, c[A0 + 0], with c[0] to c[15] holding distinct marker colors and c[191] magenta. The rd "
    "cell shows the color read, the hw cell the color hardware read (black where hardware read zero). On hardware rd "
    "matches hw in every row. Rows (input: constant read on hardware):"
)


def emit_tables(suites, arl_tests) -> list[str]:
    out = ["// clang-format off"]
    shaders = sorted({shader for tests in suites.values() for _, shader, _, _ in tests})
    for shader in shaders:
        out += [
            f"static const uint32_t {shader}[] = {{",
            f'#include "vertex_shader_exceptional/{shader_file(shader)}.vshinc"',
            "};",
        ]
    out.append("")
    for suite, tests in suites.items():
        for test_name, _shader, records, _title in tests:
            out.append(f"static constexpr VshExcRecord {ident(suite, test_name)}Records[] = {{")
            for r in records:
                out.append(
                    f'    {{"{r.label}", {c_vec(r.a)}, {c_vec(r.b)}, {c_vec(r.c)}, {COMPONENTS.index(r.lane)}, '
                    f"0x{r.hw:08X}}},"
                )
            out.append("};")
        out.append("")
        out.append(f"static constexpr VshExcTestCase {ident(suite, 'Tests')}[] = {{")
        for test_name, shader, _records, title in tests:
            rec = f"{ident(suite, test_name)}Records"
            out.append(
                f'    {{"{test_name}", "{title}", {shader}, sizeof({shader}), {rec}, sizeof({rec}) / sizeof({rec}[0])}},'
            )
        out.append("};")
        out.append("")
    for test_name, records in arl_tests:
        out.append(f"static constexpr VshExcArlRecord {ident('ilu', test_name)}Records[] = {{")
        for label, a, index, _ in records:
            reads_zero = "true" if index is None else "false"
            out.append(f'    {{"{label}", {c_vec(a)}, {reads_zero}, {0 if index is None else index}}},')
        out.append("};")
    out.append("")
    out.append("static constexpr VshExcArlTestCase kIluArlTests[] = {")
    for test_name, _ in arl_tests:
        rec = f"{ident('ilu', test_name)}Records"
        out.append(f'    {{"{test_name}", {rec}, sizeof({rec}) / sizeof({rec}[0])}},')
    out.append("};")
    out.append("// clang-format on")
    return out


def emit_docs(suites, arl_tests, suite) -> list[str]:
    intro = f"{SUITE_INTROS[suite]} The layout is described in VertexShaderExceptionalMACTests."
    lines = ["/**"] + [" * " + x for x in textwrap.wrap(intro, 114)] + [" *"]

    def add(test_name, text, descriptions):
        lines.append(f" * @tc {test_name}")
        lines.extend(" *  " + x for x in textwrap.wrap(text, 113))
        lines.extend(f" *   {d}" for d in descriptions)
        lines.append(" *")

    for test_name, _shader, records, title in suites[suite]:
        text = (
            f"{title}. On hardware the hw and rf cells are green and the SGE row green, red, red. Rows (label: "
            "hardware result):"
        )
        add(test_name, text, [r.description for r in records])
    if suite == "ilu":
        for test_name, records in arl_tests:
            add(test_name, ARL_DOC, [d for _, _, _, d in records])
    lines[-1] = " */"
    return lines


def emit_constructor(suite: str, class_name: str) -> list[str]:
    tests = ["kIluTests", "kIluArlTests"] if suite == "ilu" else [ident(suite, "Tests")]
    indent = " " * (len(class_name) * 2 + 3)
    out = [
        f"{class_name}::{class_name}(TestHost &host, std::string output_dir,",
        f"{indent}const Config &config)",
        "    : TestSuite(host, std::move(output_dir), kSuiteName, config), renderer_(host) {",
    ]
    for tests_name in tests:
        out += [
            f"  for (const auto &test_case : {tests_name}) {{",
            "    tests_[test_case.test_name] = [this, &test_case]() {",
            "      renderer_.Draw(test_case);",
            "      FinishDraw(test_case.test_name);",
            "    };",
            "  }",
        ]
    out.append("}")
    return out


def emit_cpp(suites, arl_tests, revision: str) -> str:
    out = [
        "// Generated by utils/generate_vsh_exceptional_cases.py from a hardware log of the VSH Exceptional tests in",
        f"// nxdk_low_level_nv2a_tests {revision}. Do not edit.",
        "//",
        "// The tables hold raw bit patterns that must never pass through host float handling (x87 loads quiet",
        "// signaling NaNs), so every value is a uint32_t.",
        "",
        "#include <string>",
        "#include <utility>",
        "",
        '#include "vertex_shader_exceptional_value_tests.h"',
        "",
    ]
    out += emit_tables(suites, arl_tests)
    for suite, class_name in SUITES.items():
        out.append("")
        out += emit_docs(suites, arl_tests, suite)
        out += emit_constructor(suite, class_name)
    return "\n".join(out) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument(
        "log",
        type=Path,
        help="vsh_log.json from a hardware run of nxdk_low_level_nv2a_tests",
    )
    parser.add_argument(
        "--allow-dirty",
        action="store_true",
        help="accept a log from a test build with uncommitted changes, for experiments only",
    )
    args = parser.parse_args()

    with args.log.open(encoding="utf-8") as infile:
        revision, data = load_records(json.load(infile), allow_dirty=args.allow_dirty)
    suites, arl_tests = build(data)
    for tests in suites.values():
        for _, _, records, _ in tests:
            for r in records:
                if len(r.label) > MAX_LABEL:
                    sys.exit(f"Label longer than {MAX_LABEL} characters: {r.label}")
    with _OUT_PATH.open("w", encoding="ascii", newline="\n") as f:
        f.write(emit_cpp(suites, arl_tests, revision))


if __name__ == "__main__":
    main()
