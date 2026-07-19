# Progress reporting on decomp.dev

[decomp.dev](https://decomp.dev) is the decomp-scene progress dashboard (history
graph, treemap, per-category breakdown, coverage shields). This doc explains how
it actually ingests progress, the tooling we added to feed it, and how to stand
up a **self-hosted** instance (the chosen setup) pointed at this project.

## How decomp.dev ingests progress (the one contract that matters)

decomp.dev is **artifact-driven**. It does **not** read `decomp.yaml` /
`decomp_settings` (that file is for coddog and the wider ecosystem, not for
decomp.dev — verified by reading the whole `encounter/decomp.dev` source). The
pipeline is:

1. Your project is a **GitHub repo** registered on a decomp.dev instance.
2. A **GitHub Actions** run produces an **artifact** whose name matches
   `^<version>[_-]report(...)?$` — for us **`jp_report`** (`jp` is the version
   name in `decomp.yaml`).
3. Inside the artifact zip there is a file whose stem is `report`,
   `<version>_report`, or `progress` — for us **`report.json`**.
4. decomp.dev's bot lists the repo's workflow runs (per commit), downloads that
   artifact, and calls objdiff's `Report::parse` (JSON if the first byte is `{`,
   otherwise protobuf), then `migrate()` (which just validates the report is
   **version 2**), and inserts the measures **per commit**. A periodic scheduler
   plus a manual **Refresh** button in the manage UI trigger this.

So the entire integration reduces to: **publish a valid objdiff v2 `Report` as an
artifact named `jp_report/report.json` from CI.** That is what we now do.

There is **no** local-file or HTTP API to push a report — I checked: the CLI only
has a `changes` command, and project registration (`/manage/new`) requires a real
GitHub repo (it calls the GitHub API, checks admin permission, and scans Actions
for the report workflow). **GitHub Actions is the only transport, even
self-hosted.** The self-host win is that with your instance's token you can point
it at a **private** repo and keep the dashboard private (see below).

## The objdiff report format

The report is objdiff's `Report` protobuf, JSON-encoded (schema:
[`report.proto`](https://github.com/encounter/objdiff/blob/main/objdiff-core/protos/report.proto)).
Shape (verified by round-tripping our output through the real `report.proto` with
`protoc`, and against a live decomp.dev report):

```jsonc
{
  "measures": { /* aggregate over all units */ },
  "units":    [ /* one per function (our granularity) */ ],
  "version":  2,
  "categories": [ { "id": "game", "name": "Game code", "measures": {…} }, … ]
}
```

Encoding gotchas (proto3 JSON mapping — get these wrong and `Report::parse`
rejects it): **u64 fields are JSON strings** (`total_code`, `matched_code`,
`complete_code`, `size`, `address`), **u32 and float fields are numbers**
(`total_functions`, `*_percent`, `total_units`), field names are **snake_case**,
and default (0) fields may be omitted. Each unit carries `functions[]`
(`{name, size, fuzzy_match_percent, address}`) and
`metadata.progress_categories` (which categories it rolls into).

## Our tooling

- **`tools/objdiff-report.py`** — generates the report. It is **build-free**: like
  `tools/progress.py` it derives everything from committed config (no nix / cc1 /
  reassembly), so CI needs only Python.
  - "Matched" is defined identically to `tools/progress.py` (carved `c`
    subsegment **and** no `INCLUDE_ASM`), matched by address via the symbol table
    — the two tools always agree.
  - One objdiff **unit per function** (per-function treemap). Units are tagged
    into the `game` / `sdk` progress categories using the same `SDK_START`
    boundary as `progress.py`.
  - The scaffolded sibling executables each get **their own category** —
    `menu` (MENU.EXE), `ending` (ENDING.EXE), `trial` (TRIAL.EXE) — built
    from the committed `config/functions.<name>.tsv` inventories, with units
    named `<exe>/<function>`, so decomp.dev shows per-binary progress the
    moment functions start matching. The console summary of `./Build report`
    prints the same per-binary breakdown followed by the total.
  - Default output is **game code only** for main.exe (top-level % = whole
    multi-exe progress once the sibling categories are included).
    `--include-sdk` also emits the differently-compiled PsyQ SDK block.
  - **Partial progress.** A byte-exact function is 100% (`complete`); an
    `INCLUDE_ASM` stub is 0%. Functions with a `#else` NON_MATCHING draft (close
    but not byte-exact) get a real per-function `fuzzy_match_percent` from
    `config/fuzzy.main.exe.tsv` — so decomp.dev shows them as partially done
    instead of 0%. Following objdiff's semantics, `matched_code` is fuzzy-weighted
    (partial credit; this is the number that rises with partials) while
    `complete_code` counts only byte-exact functions. `--no-fuzzy` emits the
    strict binary 0/100 report.
  - Flags: `--out PATH` (default `.shake/build/tenchu/report.json`), `--stdout`,
    `--include-sdk`, `--no-fuzzy`, `--functions PATH`.
- **`./Build report`** — Shake phony that runs the generator.
- **`tools/fuzz-score.py` / `./Build fuzz-score`** — computes the per-function
  fuzzy percentages into `config/fuzzy.main.exe.tsv`. For each NON_MATCHING draft
  it builds the draft (`NON_MATCHING=<name> ./Build`), disassembles the original
  vs our compiled draft, and takes an instruction-sequence similarity ratio (an
  approximation of objdiff's fuzzy match, reusing `asmdiff.py`'s machinery; a
  non-exact draft is capped below 100). **Not build-free and slow** (one
  incremental build per draft). Run `tools/fuzz-score.py --only <Name>` after
  every guarded-draft edit and after exact promotion (which removes the stale
  row), then gate with `tools/fuzz-score.py --check`. Each row carries the
  source SHA-256; report generation refuses missing, orphaned, or source-stale
  rows. Build failures are recorded with an empty score and remain at 0%.
- **`config/functions.main.exe.tsv`** — a **committed, reviewed snapshot** of the
  function inventory (`addr<TAB>size<TAB>name`). The live source
  (`.shake/ghidra-export/functions.tsv`) is gitignored (a local Ghidra export), so
  this snapshot makes report generation work on a fresh checkout / in CI. Report
  generation never overwrites it: the snapshot carries corrected `LoadCard` and
  `FUN_800593a0` extents that Ghidra still truncates. Audit a new export explicitly
  with `tools/objdiff-report.py --functions .shake/ghidra-export/functions.tsv` and
  review inventory changes before updating the snapshot.
- **`.github/workflows/report.yml`** — runs the generator and uploads the
  `jp_report` artifact (containing `report.json`) on pushes to the default branch
  (set to `master`, with `main` as a fallback). Adjust `branches:` if yours differs.

Regenerate locally any time:

```console
$ ./Build report
report (game): 555 units, 322/555 functions, 89900/302824 bytes (29.69%) -> .shake/build/tenchu/report.json
```

## Going live (self-hosted)

Even self-hosted, decomp.dev reads reports from a GitHub repo's Actions. So:

1. **Create a GitHub repo and push this project.** A **private** repo is fine (and
   is the point of self-hosting). `disks/` (the ROM and original `main.exe`) is
   gitignored, so nothing copyrighted is committed. Set `decomp.yaml`'s `repo:`
   to the new `owner/name` (cosmetic — decomp.dev ignores it, but coddog uses it).
2. **Make the workflow run.** Ensure `.github/workflows/report.yml`'s `branches:`
   matches your default branch, push a commit, and confirm the **`jp_report`**
   artifact appears under the run's Artifacts. (The job is ~20s — build-free.)
3. **Run your decomp.dev instance** (from `encounter/decomp.dev`; Rust backend +
   `npm` frontend, per its README). In `config.yml`:
   - `server.dev_mode: true` lets you log in as super admin without GitHub OAuth.
   - `github.token`: a Personal Access Token. For a **private** repo it needs the
     classic **`repo`** scope (read access to the repo **and its Actions
     artifacts**); `public_repo` suffices for a public repo. This token is how the
     instance downloads your artifacts.
4. **Register the project:** browse to `http://localhost:3000/manage/new`, enter
   the repo URL. decomp.dev fetches the repo, detects the report workflow, and
   pulls existing report artifacts. Progress appears immediately; the history
   graph fills in as you push more commits. Use the **Refresh** button on the
   manage page (or wait for the scheduler) to pull new reports on demand.

> **Private repo ⇒ self-host.** The *hosted* decomp.dev at decomp.dev cannot read
> a private repo's artifacts, so a private repo only works with your own instance
> (whose token has access). If you later make the repo public you can additionally
> register it on the hosted site (optionally installing the `decomp-dev` GitHub App
> for automatic refresh + PR progress comments).

## Follow-ups / ideas

- **Nicer treemap via real TUs.** We emit one unit per function today. The demo
  disc's `PSX.SYM` gives the original source-file → function map
  ([`psx-sym.md`](psx-sym.md)); grouping functions into their real source files
  (with a fallback unit for the rest) would produce a source-tree treemap like
  sotn-decomp's. This is the deferred "group by PSX.SYM" option.
- **Finer categories.** Subdivide `game` into subsystems (item / AI / render /
  file) using the `PSX.SYM` TU map, for per-subsystem progress lines.
