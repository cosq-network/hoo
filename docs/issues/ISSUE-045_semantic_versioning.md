# ISSUE-045 Semantic Versioning, Version Bumping, Linux Build Pipelines & GitHub Release Workflow

## Status: ✅ IMPLEMENTED

---

## Overview
This issue tracks the implementation of a **robust semantic versioning** strategy for the Hoo project, automated **version bumping**, a **Linux CI/CD pipeline**, and a **GitHub Release workflow** that publishes artifacts (binaries, docs, and changelog) for each tagged version.

---

## Motivation
* **Predictable releases** – Consumers need to know whether a new release contains breaking changes, new features, or bug fixes.
* **Automation** – Manual version updates are error‑prone. An automated bump reduces human overhead and ensures consistency between `README`, `CMakeLists.txt`, package manifests, and Git tags.
* **Cross‑platform CI** – The project previously built on macOS only. Supporting Linux expands the contributor base and allows publishing Linux binaries.
* **GitHub Release automation** – Publishing compiled binaries and a generated changelog directly from CI improves discoverability and accelerates adoption.

---

## Semantic Versioning Rules

The project follows **Semantic Versioning 2.0.0** (`MAJOR.MINOR.PATCH`):

| Bump Level | Trigger (conventional commit prefix) | Example |
|------------|--------------------------------------|---------|
| **MAJOR** | `BREAKING CHANGE:` anywhere in message body, or message starts with `BREAKING:` | Resets minor and patch to `0` |
| **MINOR** | Message starts with `feat:` or `feat(<scope>):` | `feat(parser): add async fn support` |
| **PATCH** | Any other type (`fix:`, `chore:`, `refactor:`, `docs:`, `test:`, `perf:`, `ci:`) | `fix(jit): correct namespace for HVMJIT` |

### Bumping the Major Version

**Method 1 – Explicit flag:**
```bash
python3 scripts/bump_version.py major
```
Forces a major bump regardless of git log. Example: `0.1.0` → `1.0.0`.

**Method 2 – Auto-detect via `BREAKING CHANGE` commit:**
Write a commit that includes `BREAKING CHANGE:` in the footer or starts with `BREAKING:`:
```
feat(runtime): redesign hoo_readchar API

BREAKING CHANGE: hoo_readchar now returns HooCharacter* instead of int64_t.
All callers must be updated.
```
Then run:
```bash
python3 scripts/bump_version.py auto   # detects BREAKING CHANGE → major bump
```

In `auto` mode the script scans `git log` since the last tag, checks for `BREAKING CHANGE` first, then `feat:`, then falls back to `patch`.

---

## Implemented Files

| File | Description |
|------|-------------|
| `scripts/bump_version.py` | Conventional-commit aware version bumper |
| `scripts/generate_changelog.py` | Categorised Markdown release-notes generator |
| `.github/workflows/linux-build.yml` | Linux (Ubuntu) CI pipeline |
| `.github/workflows/release.yml` | GitHub Release workflow (Linux + macOS matrix) |
| `docs/CHANGELOG.md` | Initial changelog (v0.1.0) |
| `docs/DEVELOPMENT.md` | Developer guide: versioning, CI, conventional commits |

---

## Version Bump Script (`scripts/bump_version.py`)

### Usage
```bash
python3 scripts/bump_version.py [major|minor|patch|auto]
```

### What it does
1. Reads current version from `CMakeLists.txt` (`project(Hoo VERSION X.Y.Z)`).
2. Scans `git log` since the last tag (in `auto` mode) to determine bump level.
3. Calculates the new version:
   - **major**: `MAJOR+1 . 0 . 0`
   - **minor**: `MAJOR . MINOR+1 . 0`
   - **patch**: `MAJOR . MINOR . PATCH+1`
4. Updates:
   - `CMakeLists.txt` – version in `project()` and `HOO_VERSION` variable.
   - `docs/CHANGELOG.md` – prepends a new `## X.Y.Z – YYYY-MM-DD` section.
   - `README.md` – updates the shields.io version badge URL.
5. Commits: `chore: bump version to X.Y.Z`
6. Tags: `vX.Y.Z`

Push to GitHub to trigger the release pipeline:
```bash
git push origin main --tags
```

---

## Changelog Generator (`scripts/generate_changelog.py`)

### Usage
```bash
python3 scripts/generate_changelog.py <new_tag> [<old_tag>]
```
If `<old_tag>` is omitted, the script auto-detects the previous tag.

### Output categories
| Category | Commits |
|----------|---------|
| 💥 Breaking Changes | `BREAKING CHANGE:` / `BREAKING:` |
| ✨ New Features | `feat:` |
| 🐛 Bug Fixes | `fix:` |
| ⚡ Performance | `perf:` |
| ♻️ Refactoring | `refactor:` |
| 📝 Documentation | `docs:` |
| 🧪 Tests | `test:` |
| 🔧 Chores | `chore:` |
| 🗂 Other | Anything else |

---

## Linux Build Pipeline (`.github/workflows/linux-build.yml`)

- **Trigger:** push/PR to `main` or `develop`.
- **Runner:** `ubuntu-latest`
- **Matrix:** `Debug` and `Release` build types.
- **Dependencies:** LLVM 22, ANTLR4 runtime, GoogleTest, libuv (cached).
- **Artifacts:** `hoo-linux-x86_64.tar.gz` (Release only, retained 30 days).

---

## GitHub Release Workflow (`.github/workflows/release.yml`)

- **Trigger:** tag push matching `v*`, or manual `workflow_dispatch`.
- **Matrix:** `ubuntu-latest` (Linux x86_64) and `macos-latest` (macOS arm64).
- **Steps:**
  1. Install platform-specific dependencies.
  2. Configure and build Release binaries.
  3. Run full test suite on each platform.
  4. Validate binary (`./hoo --version`).
  5. Package into `hoo-linux-x86_64.tar.gz` / `hoo-macos-arm64.tar.gz`.
  6. Generate release notes via `scripts/generate_changelog.py`.
  7. Publish GitHub Release with all binary assets attached.
- **Pre-release detection:** tags containing `-` (e.g. `v1.0.0-alpha`) are marked as pre-releases automatically.

---

## CMakeLists.txt Changes

The `project()` declaration now includes the version:
```cmake
project(Hoo
    VERSION 0.1.0
    DESCRIPTION "Hoo – a high-performance statically-typed systems programming language"
    LANGUAGES C CXX
)

set(HOO_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(HOO_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(HOO_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(HOO_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
```

---

## README.md Changes

Badges added at the top:
```markdown
[![Version](https://img.shields.io/badge/version-0.1.0-blue)](...)
[![macOS Build](...build-and-test.yml/badge.svg)](...)
[![Linux Build](...linux-build.yml/badge.svg)](...)
[![License](https://img.shields.io/github/license/cosq-network/hoo)](LICENSE)
```

---

## Risks & Mitigations
| Risk | Impact | Mitigation |
|------|--------|------------|
| Incorrect version bump due to malformed commit messages | Wrong version released | Enforce `commitlint` in PR checks. |
| Linux CI environment differs from developer machines | Hard-to-reproduce failures | Pin LLVM 22 via cached download; Dockerfile available. |
| Release workflow publishes broken artifacts | Users download non-functional binaries | `./hoo --version` validation step before packaging. |
| Changelog generation misses entries | Incomplete release notes | `git fetch --tags --prune` before generation; fallback to raw `git log`. |

---

## Acceptance Criteria
1. ✅ `scripts/bump_version.py` updates `CMakeLists.txt`, `CHANGELOG.md`, `README.md`, commits, and tags.
2. ✅ **Major bump** triggered by `BREAKING CHANGE:` in commit body or explicit `major` argument.
3. ✅ Linux CI runs on every push/PR, builds Debug+Release, runs full test suite, archives binaries.
4. ✅ Tag push `vX.Y.Z` creates a GitHub Release with Linux + macOS binaries and generated changelog.
5. ✅ `docs/DEVELOPMENT.md` documents the entire workflow for contributors.
6. ✅ `README.md` shows live version, build status, and license badges.
7. ✅ Existing macOS CI pipeline (`build-and-test.yml`) is unchanged.

---

*Prepared by Antigravity AI – 2026‑06‑21 | Implemented: 2026‑06‑21*
