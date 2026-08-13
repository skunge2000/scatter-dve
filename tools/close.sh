#!/usr/bin/env bash
# Close a work unit: build, test, and on success tag.
#
#   ./tools/close.sh NN
#
# e.g.  ./tools/close.sh 01
#
# Refuses on a dirty tree, a failed build, a failed test, or an existing tag.
# Only green work gets tagged; the tag is the recovery point.

set -uo pipefail
cd "$(dirname "$0")/.." || exit 1

if [[ $# -lt 1 ]]; then
    echo "usage: ./tools/close.sh NN     (e.g. ./tools/close.sh 01)" >&2
    exit 2
fi

NN="$1"
TAG="wu-${NN}-green"

if [[ -n "$(git status --porcelain)" ]]; then
    echo "ERROR: uncommitted changes. Commit them first:" >&2
    git status --short >&2
    exit 1
fi

if git rev-parse -q --verify "refs/tags/$TAG" >/dev/null; then
    echo "ERROR: tag $TAG already exists." >&2
    exit 1
fi

echo "=== configure ==="
cmake -B build -G Ninja || exit 1

echo "=== build ==="
cmake --build build || { echo "BUILD FAILED — not tagging." >&2; exit 1; }

echo "=== test ==="
if ! ctest --test-dir build --output-on-failure; then
    echo >&2
    echo "TESTS FAILED — not tagging." >&2
    echo "Record the failure verbatim in HANDOFF.md, commit, and resume" >&2
    echo "next session. See docs/workflow.md section 3." >&2
    exit 1
fi

git tag -a "$TAG" -m "WU-${NN}: build green, tests pass"
echo "=== tagged $TAG ==="

if git remote | grep -qx origin; then
    git push origin HEAD --tags \
        || echo "WARNING: push failed; commit is local only." >&2
fi

echo
echo "WU-${NN} closed green."
echo "Recovery point: git reset --hard ${TAG}"
echo "Remaining: update HANDOFF.md for the next unit if the session did not."
