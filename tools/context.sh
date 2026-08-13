#!/usr/bin/env bash
# Emit the session-opening context bundle.
#
#   ./tools/context.sh [source files...]
#
# Always includes the five state files, git position and test status.
# Named source files are appended verbatim. Take the file list for the
# current work unit from WORK-UNITS.md.

set -uo pipefail
cd "$(dirname "$0")/.." || exit 1

emit() {
    local path="$1"
    if [[ -f "$path" ]]; then
        printf '\n===== FILE: %s =====\n' "$path"
        cat "$path"
    else
        printf '\n===== FILE: %s (ABSENT) =====\n' "$path"
    fi
}

printf '===== SESSION CONTEXT: scatter-dve =====\n'
printf 'generated: %s\n' "$(date -u '+%Y-%m-%dT%H:%M:%SZ')"
printf 'host: %s (%s)\n' "$(uname -s)" "$(uname -m)"

printf '\n----- git -----\n'
if git rev-parse --git-dir >/dev/null 2>&1; then
    printf 'describe: %s\n' "$(git describe --tags --always --dirty 2>/dev/null || echo none)"
    printf 'branch:   %s\n' "$(git rev-parse --abbrev-ref HEAD 2>/dev/null)"
    printf 'head:     %s\n' "$(git log -1 --format='%h %s' 2>/dev/null)"
    printf '\nuncommitted:\n'
    git status --porcelain 2>/dev/null | sed 's/^/  /' || true
else
    printf 'not a git repository\n'
fi

printf '\n----- tests -----\n'
if [[ -d build ]]; then
    ( cd build && ctest --output-on-failure 2>&1 | tail -n 25 ) \
        || printf '(ctest returned non-zero)\n'
else
    printf 'no build directory\n'
fi

printf '\n----- tree -----\n'
if command -v git >/dev/null 2>&1 && git rev-parse --git-dir >/dev/null 2>&1; then
    git ls-files | grep -E '^(src|tests|tools)/' | sed 's/^/  /' || true
else
    find src tests tools -type f 2>/dev/null | sort | sed 's/^/  /' || true
fi

for f in HANDOFF.md INVARIANTS.md DECISIONS.md CORRECTIONS.md WORK-UNITS.md; do
    emit "$f"
done

for f in "$@"; do
    emit "$f"
done

printf '\n===== END CONTEXT =====\n'
