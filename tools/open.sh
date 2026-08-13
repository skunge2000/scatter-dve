#!/usr/bin/env bash
# Session open: build the context bundle and put it on the clipboard.
#
#   ./tools/open.sh [source files...]
#
# File list for the current work unit is stated in WORK-UNITS.md.

set -uo pipefail
cd "$(dirname "$0")/.." || exit 1

BUNDLE="$(mktemp "${TMPDIR:-/tmp}/scatter-ctx.XXXXXX")"
trap 'rm -f "$BUNDLE"' EXIT

./tools/context.sh "$@" > "$BUNDLE"

BYTES=$(wc -c < "$BUNDLE" | tr -d ' ')
WORDS=$(wc -w < "$BUNDLE" | tr -d ' ')

if command -v pbcopy >/dev/null 2>&1; then
    pbcopy < "$BUNDLE"
    COPIED="clipboard (pbcopy)"
elif command -v xclip >/dev/null 2>&1; then
    xclip -selection clipboard < "$BUNDLE"
    COPIED="clipboard (xclip)"
elif command -v wl-copy >/dev/null 2>&1; then
    wl-copy < "$BUNDLE"
    COPIED="clipboard (wl-copy)"
else
    cp "$BUNDLE" ./session-context.txt
    COPIED="./session-context.txt (no clipboard tool found)"
fi

printf 'Context bundle: %s bytes, ~%s words\n' "$BYTES" "$WORDS"
printf 'Copied to: %s\n' "$COPIED"

if [[ "$WORDS" -gt 12000 ]]; then
    printf '\nWARNING: bundle is large. Check you are not passing more files\n'
    printf 'than the current work unit needs (see WORK-UNITS.md).\n'
fi

CURRENT_WU=$(grep -m1 -E '^\*\*(Next work unit|Session)' HANDOFF.md 2>/dev/null || true)
printf '\nHANDOFF says: %s\n' "${CURRENT_WU:-(could not parse HANDOFF.md)}"
printf 'Paste the bundle, then state the work unit ID.\n'
