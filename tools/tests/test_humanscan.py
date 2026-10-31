from contextlib import redirect_stdout
import io
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from tools import humanscan


class HumanScanTests(unittest.TestCase):
    def test_reports_scaffolding_without_counting_comment_or_string_examples(self):
        samples = {
            "empty_loop": "do { } while (0);",
            "discarded_volatile": "(void)*(Actor *volatile *)&CurrentActor;",
            "cancelled_update": "angle += next_angle - angle;",
            "identical_arms": "if (value) { result = next; } else { result = next; }",
            "unknown_field": "u16 unknown0a;",
        }
        patterns = dict(humanscan.PATTERNS)
        for kind, source in samples.items():
            with self.subTest(kind=kind):
                self.assertRegex(humanscan.strip_comments(source), patterns[kind])
                examples = f'/* {source} */\nconst char *example = "{source}";'
                self.assertNotRegex(humanscan.strip_comments(examples), patterns[kind])
        self.assertNotRegex("angle += jitter - bias;", patterns["cancelled_update"])
        self.assertNotRegex("if (value) { a = next; } else { a = previous; }",
                            patterns["identical_arms"])

    def test_release_macros_do_not_hide_a_standalone_empty_loop(self):
        source = (
            "#define DBG(x) do { } while (0)\n"
            "#define TRACE(x) \\\n"
            "    do { } while (0)\n"
            "void work(void) { do { } while (0); }\n"
        )
        pattern = dict(humanscan.PATTERNS)["empty_loop"]
        self.assertEqual(len(pattern.findall(humanscan.without_directives(source))), 1)

    def test_shared_includes_follow_uppercase_c_and_inc_once_even_with_cycles(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            wrapper = root / "wrapper.c"
            shared = root / "SHARED.C"
            nested = root / "nested.inc"
            wrapper.write_text('#include "SHARED.C"\n#include "ignored.h"\n')
            shared.write_text('#include "nested.inc"\n')
            nested.write_text('#include "SHARED.C"\n')
            self.assertEqual(list(humanscan.with_shared_sources([wrapper, shared])),
                             [wrapper.resolve(), shared.resolve(), nested.resolve()])

    def test_target_and_headers_include_the_shared_implementation(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            main = root / "main.exe"
            trial = root / "trial.exe"
            main.mkdir()
            trial.mkdir()
            (main / "SHARED.C").write_text("void work(void) { do {} while (0); }\n")
            (main / "unrelated.c").write_text("void other(void) { do {} while (0); }\n")
            (trial / "wrapper.c").write_text('#include "../main.exe/SHARED.C"\n')
            (trial / "layout.h").write_text("struct Layout { int unknown10; };\n")
            output = io.StringIO()
            with patch.object(humanscan, "SRC", main), patch(
                "sys.argv", ["humanscan.py", "--target", "trial.exe", "--headers"]
            ), redirect_stdout(output):
                humanscan.main()
            self.assertIn("../main.exe/SHARED.C", output.getvalue())
            self.assertIn("layout.h", output.getvalue())
            self.assertNotIn("unrelated.c", output.getvalue())


if __name__ == "__main__":
    unittest.main()
