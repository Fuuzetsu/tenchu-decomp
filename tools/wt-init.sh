#!/usr/bin/env bash
# Make a fresh git worktree usable for matching.
#
# `disks/` (the game images) and `.shake/` (the build dir, which also holds the
# Ghidra export) are both gitignored, so a new worktree gets neither. Without
# `disks/` nothing builds; without `.shake/ghidra-export/functions.tsv`,
# tools/matcher-prompt.py, coverage.py, triage.py and findsimilar.py all die with
# FileNotFoundError.
#
# Symlink both from the primary worktree. The images are read-only and the Ghidra
# export is regenerated only by an explicit Ghidra run, so sharing them is safe;
# everything the build writes still goes into this worktree's own .shake/.
#
#   tools/wt-init.sh          # run from inside the new worktree
#
# Idempotent.
set -euo pipefail

here=$(git rev-parse --show-toplevel)
# The first `git worktree list` entry is always the primary worktree.
# Do not `exit` awk after the first match: with `set -o pipefail`, closing a
# long `git worktree list` pipe early makes git die with SIGPIPE and aborts this
# script before it creates any links.
main=$(git worktree list --porcelain | awk '
  /^worktree / && !found { print $2; found = 1 }
')

if [ "$here" = "$main" ]; then
  echo "wt-init: already in the primary worktree ($here); nothing to do"
  exit 0
fi

link() {
  local rel=$1 src="$main/$1" dst="$here/$1"
  if [ ! -e "$src" ]; then
    echo "wt-init: WARNING: $src does not exist in the primary worktree; skipping"
    return
  fi
  if [ -e "$dst" ] || [ -L "$dst" ]; then
    echo "wt-init: $rel already present"
    return
  fi
  mkdir -p "$(dirname "$dst")"
  ln -s "$src" "$dst"
  echo "wt-init: linked $rel -> $src"
}

# NOT `link disks`: disks/ holds a TRACKED README.md, so the directory always
# exists in a fresh worktree while every image inside it is missing. Link the
# entries, not the directory.
if [ -d "$main/disks" ]; then
  for src in "$main"/disks/*; do
    [ -e "$src" ] || continue
    name=$(basename "$src")
    [ "$name" = "README.md" ] && continue
    link "disks/$name"
  done
else
  echo "wt-init: WARNING: no disks/ in the primary worktree"
fi

link .shake/ghidra-export

# Staleness check. An agent worktree is often branched from an OLD commit: lanes
# in one rollout started 26-36 commits behind, one of them missing the entire
# checkpoint its task described (a bare INCLUDE_ASM stub), and another missing a
# tool its task told it to run first. The contract mandates checking this by
# hand; do it here so nobody has to remember.
base=$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo "?")
# NOTE the bootstrap hole this cannot close by itself: a worktree branched from a
# commit OLDER than this check runs the OLD script and is never warned. A lane 42
# commits stale hit exactly that. matcher-prompt.py carries the same check for
# that reason — it runs from the PRIMARY worktree, so it is always current.
if git rev-parse --verify --quiet master >/dev/null; then
  behind=$(git rev-list --count HEAD..master 2>/dev/null || echo 0)
  ahead=$(git rev-list --count master..HEAD 2>/dev/null || echo 0)
  if [ "${behind:-0}" -gt 0 ]; then
    echo
    echo "wt-init: *** YOUR BRANCH ($base) IS $behind COMMIT(S) BEHIND master ***"
    if [ "${ahead:-0}" -eq 0 ]; then
      echo "wt-init: you have no commits yet, so fast-forward BEFORE any edit:"
      echo "wt-init:     git merge --ff-only master"
    else
      echo "wt-init: you have $ahead commit(s) of your own — do NOT ff blindly;"
      echo "wt-init: rebase or report, and re-measure your baseline either way."
    fi
    echo "wt-init: then re-measure: a stale base can hide the checkpoint your"
    echo "wt-init: task describes, or a tool it tells you to run."
    echo
  else
    echo "wt-init: branch $base is up to date with master"
  fi
fi

echo "wt-init: ready. Verify with: nix develop --command bash -c './Build check'"
