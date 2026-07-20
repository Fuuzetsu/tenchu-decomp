from __future__ import annotations

import unittest

from tools import redux_typed_debugger as rtd


class ParseDeclarationTests(unittest.TestCase):
    def test_pointer_to_struct_uses_bare_base_and_trailing_star(self) -> None:
        # The widget dereferences a pointer by stripping the trailing " *",
        # then matches the base against imported struct names.
        self.assertEqual(
            rtd.parse_declaration("struct ModelType *model"),
            ("ModelType *", "model"),
        )

    def test_unsigned_primitives_normalise_to_widget_tokens(self) -> None:
        self.assertEqual(
            rtd.parse_declaration("unsigned char active"), ("u_char", "active")
        )
        self.assertEqual(
            rtd.parse_declaration("unsigned long flags"), ("u_long", "flags")
        )

    def test_array_folds_dimension_into_type(self) -> None:
        self.assertEqual(
            rtd.parse_declaration("char actbuf[2]"), ("char[2]", "actbuf")
        )

    def test_void_and_empty_yield_no_declaration(self) -> None:
        self.assertIsNone(rtd.parse_declaration("void"))
        self.assertIsNone(rtd.parse_declaration(""))


class ParseTypesTests(unittest.TestCase):
    SOURCE = """\
struct AfterimageType {  /* size 88 */
    struct ModelType *model;  /* +0x0 */
    struct SVECTOR vector1;  /* +0x4 */
    short maxn;  /* +0x14 */
    struct POLY_GT4 poly;  /* +0x24 */
};

union Payload {  /* size 8 */
    long asLong;  /* +0x0 */
    short asShort;  /* +0x0 */
};
"""

    def test_field_sizes_come_from_consecutive_offset_deltas(self) -> None:
        structs = rtd.parse_types(self.SOURCE)
        fields = {f.name: f for f in structs["AfterimageType"].fields}
        self.assertEqual(fields["model"].size, 4)      # 0x4 - 0x0
        self.assertEqual(fields["vector1"].size, 16)   # 0x14 - 0x4
        self.assertEqual(fields["maxn"].size, 16)      # 0x24 - 0x14
        self.assertEqual(fields["poly"].size, 52)      # 88 - 0x24
        self.assertEqual(fields["model"].type, "ModelType *")
        self.assertEqual(fields["vector1"].type, "SVECTOR")

    def test_union_fields_fall_back_to_type_size(self) -> None:
        structs = rtd.parse_types(self.SOURCE)
        fields = {f.name: f for f in structs["Payload"].fields}
        self.assertEqual(fields["asLong"].size, 4)
        self.assertEqual(fields["asShort"].size, 2)

    def test_emit_types_is_semicolon_delimited_with_trailing_terminator(self) -> None:
        structs = rtd.parse_types(self.SOURCE)
        line = [l for l in rtd.emit_types(structs).splitlines()
                if l.startswith("AfterimageType;")][0]
        self.assertTrue(line.endswith(";"))
        self.assertIn("ModelType *,model,4;", line)
        self.assertIn("POLY_GT4,poly,52;", line)


class ParseProtosTests(unittest.TestCase):
    def test_protos_split_args_and_emit_addr_name_typed_args(self) -> None:
        protos = rtd.parse_protos(
            "void CreateStage(int StageNo, int CharType);  /* WORLD.C:139 */\n"
            "void PadProc(void);\n"
        )
        self.assertEqual(len(protos["CreateStage"]), 2)
        self.assertEqual(protos["CreateStage"][0].type, "int")
        self.assertEqual(protos["PadProc"], [])
        text, count = rtd.emit_funcs(
            protos, {"CreateStage": 0x8003A3A0, "PadProc": 0x8001ADA4}, {}
        )
        self.assertEqual(count, 2)
        self.assertIn("8003a3a0;CreateStage;int,StageNo,4;int,CharType,4;", text)
        self.assertIn("8001ada4;PadProc;", text)

    def test_functions_without_a_resolved_address_are_skipped(self) -> None:
        protos = rtd.parse_protos("void Ghost(void);\n")
        _text, count = rtd.emit_funcs(protos, {}, {})
        self.assertEqual(count, 0)

    def test_at_most_four_arguments_are_emitted(self) -> None:
        protos = rtd.parse_protos(
            "void F(int a, int b, int c, int d, int e, int f);\n"
        )
        text, _ = rtd.emit_funcs(protos, {"F": 0x80010000}, {})
        self.assertEqual(text.count("int,"), 4)


if __name__ == "__main__":
    unittest.main()
