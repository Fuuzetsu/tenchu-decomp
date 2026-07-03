This is a decompilation project for the PS1 game Tenchu. There are various tools
in here. The final goal is probably to implement a byte-identical
reimplementation of the game, not unlike was done for Mario 64 etc.

Read the current plan from PLAN.md . You can update this file with TODOs and
future tasks as well as mark things already done etc. The developer docs index
is docs/README.md — read it before non-trivial work; it explains the build,
toolchain decisions, modding, ISO/emulator flow, and the Ghidra bridge.

For byte-matching new functions (the core activity), follow
docs/matching-cookbook.md: it records the cc1 2.8.1 source idioms that
reproduce the original bytes (dispatch/loop/expression/stack/regalloc rules,
all verified), and the workflow — tools/reverse.py to split a function, then
iterate with tools/matchdiff.py <Name> until MATCH, then ./Build check.
Keep the cookbook updated: when matching teaches a new reusable rule, add it
there (not just to session memory), and extend the worked-example pointers.

If you need some tools, you can use ~/programming/nixpkgs to get anything you
need from nix.
