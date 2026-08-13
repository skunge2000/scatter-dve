# Workflow — machine setup, folders, git

Concrete operating instructions. `SESSION-PROTOCOL.md` says what the discipline
is; this says how to run it.

One machine: the MacBook Pro M1 Max, with the UltraStudio 4K Mini attached.
Development, tests and hardware runs all happen there (ADR-013).

---

## 1. One-time setup

```bash
xcode-select --install          # clang, git
brew install cmake ninja

mkdir -p ~/src && cd ~/src
tar xzf ~/Downloads/scatter-dve-wu00.tar.gz
cd scatter-dve
chmod +x tools/*.sh

git init -b main                # -b main matters: older git defaults to master
git add -A
git commit -m "WU-00: design, invariants, session protocol"
git tag -a wu-00-green -m "Scaffolding and frozen invariants"
```

**Do not put this under `~/Documents`, `~/Desktop`, iCloud Drive or Dropbox.**
Desktop & Documents sync is on by default on macOS. A synced `.git` directory
and a synced `build/` tree will eventually corrupt each other, and the failure
looks like random git object errors rather than anything obviously
sync-related. `~/src` is outside all sync roots.

### Media directory — outside the repo

Test patterns, captures and reference frames are large and binary. They do not
belong in git.

```bash
mkdir -p ~/src/scatter-dve-media/{patterns,captures,reference}
echo 'export SCATTER_MEDIA="$HOME/src/scatter-dve-media"' >> ~/.zshrc
```

Tests that need media check `$SCATTER_MEDIA` and skip cleanly if unset, so the
suite still runs before any patterns have been generated.

---

## 2. Folder structure

```
~/src/scatter-dve/                  the repo
├── .gitignore
├── CMakeLists.txt
├── HANDOFF.md                      ← overwritten every session
├── INVARIANTS.md                   ← frozen
├── DECISIONS.md                    ← append only
├── CORRECTIONS.md                  ← append only
├── WORK-UNITS.md                   ← the backlog
├── SESSION-PROTOCOL.md
├── docs/
│   ├── architecture.md             reference, never pasted wholesale
│   └── workflow.md                 this file
├── src/
│   ├── core/                       lattice, jacobian, binner, splat, resolve
│   │   └── shapes/
│   ├── video/                      v210, chroma, interlace, raster
│   ├── io/                         file source/sink, DeckLink
│   ├── diag/                       coverage view
│   └── app/                        main, config
├── tests/
├── tools/
│   ├── context.sh                  emit session context
│   ├── open.sh                     context.sh → clipboard
│   ├── close.sh                    build, test, tag
│   └── make_testpat.cpp
└── build/                          gitignored, out-of-source

~/src/scatter-dve-media/            NOT in git
├── patterns/                       generated .v210 test signals
├── captures/                       SDI grabs
└── reference/                      known-good output for regression
```

Two CMake targets, no options to remember:

- **`scatter-core`** — `src/core/` and `src/video/`. Links no Blackmagic SDK,
  so most tests run with nothing plugged in.
- **`scatter`** — the app. Adds `src/io/`, `src/diag/`, `src/app/`, links
  DeckLink.

---

## 3. Git conventions

Trunk only (ADR-014). No branches. Commit to `main` as you go; tag when a work
unit goes green.

| Object | Form | Example |
|---|---|---|
| Commit | `WU-NN: message` | `WU-02: v210 unpack, scalar` |
| Tag | `wu-NN-green` | `wu-02-green` |

### During a work unit

Commit freely, including broken intermediate states.

```bash
git add -A && git commit -m "WU-01: CMake skeleton"
```

### Finish green

```bash
./tools/close.sh 01
```

Builds, runs `ctest`, and only on success creates the annotated tag
`wu-01-green`. It refuses on a dirty tree, a failed build, a failed test, or an
existing tag.

### Abandon a bad session

Everything since the last green tag goes:

```bash
git log --oneline wu-00-green..HEAD    # check what you'd lose
git reset --hard wu-00-green
```

### Finish red

Don't tag. Record the failing test verbatim in `HANDOFF.md`, commit that, and
resume next session.

```bash
# edit HANDOFF.md: Tests: RED, paste the failure
git add -A && git commit -m "WU-01: red, see HANDOFF"
```

### Backup

Time Machine covers the working tree. If you want off-site copies, add a
private GitHub remote and `git push -u origin main --tags`. `close.sh` pushes
automatically if a remote named `origin` exists, and stays silent if not.

---

## 4. Moving code between machine and session

### Mac → session

```bash
./tools/open.sh src/video/v210.cpp src/video/v210.hpp tests/test_v210.cpp
```

Puts the bundle on the clipboard. Paste it, then say which work unit.

Take the file list from `WORK-UNITS.md` — it is stated per unit so the decision
is mechanical rather than a judgement about what's relevant.

### Session → Mac

Whole files arrive as a downloadable archive per work unit. Never copy-paste
source by hand; one dropped line in a header produces a compile error that
looks like a design problem.

```bash
cd ~/src/scatter-dve
tar xzf ~/Downloads/wu-01.tar.gz
git status                      # inspect what landed before staging
git add -A && git commit -m "WU-01: CMake skeleton"
```

`git status` before `git add` is the check that nothing arrived in an
unexpected path.

### What never gets pasted

- `docs/architecture.md` — quote the section, don't paste the file
- Build logs beyond the last 25 lines (`context.sh` truncates already)
- Anything from `$SCATTER_MEDIA`
- Source files not listed for the current work unit
