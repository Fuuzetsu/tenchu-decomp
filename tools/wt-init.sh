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
main=$(git worktree list --porcelain | awk '/^worktree /{print $2; exit}')

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

link disks
link .shake/ghidra-export

echo "wt-init: ready. Verify with: nix develop --command bash -c './Build check'"
