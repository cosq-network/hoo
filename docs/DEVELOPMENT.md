# Development Guide

## Versioning

This project uses **Semantic Versioning 2.0.0** (`MAJOR.MINOR.PATCH`):

| Bump | Trigger |
|------|---------|
| **MAJOR** | Commit messages containing `BREAKING CHANGE:` or starting with `BREAKING:` |
| **MINOR** | A `release/*` branch is merged into `main` |
| **PATCH** | A `hotfix/*` branch is merged into `main` |

The bump level is derived from the conventional-commit prefixes in the merged
commit set:

| Prefix in commits since the last tag | Level |
|--------------------------------------|-------|
| `BREAKING CHANGE:` or `BREAKING:`    | MAJOR |
| `feat:`                              | MINOR |
| `fix:`, `chore:`, `refactor:`, etc.  | PATCH |

## Bumping the Version

The version is bumped **automatically by CI** when a `release/*` or `hotfix/*`
branch is merged into `main`, so in normal operation you should not need to run
the script yourself. It can also be run manually from the project root:

```bash
# GitFlow modes (used by CI):
python scripts/bump_version.py release   # release merge -> minor (or major on BREAKING)
python scripts/bump_version.py hotfix    # hotfix merge  -> patch (or major on BREAKING)

# Explicit levels
python scripts/bump_version.py major
python scripts/bump_version.py minor
python scripts/bump_version.py patch

# Automatically detect bump level from git log since last tag
python scripts/bump_version.py auto
```

The script will:
1. Read the current version from `CMakeLists.txt`.
2. Scan git log since the last tag to determine the bump level.
3. Update:
   - `CMakeLists.txt` – `project(Hoo VERSION X.Y.Z)`
   - `docs/CHANGELOG.md` – prepend a new version section with commit messages
   - `README.md` – update the version badge URL
   - `vcpkg.json` – the `version-string`
   - `src/hvm/HOModule.h` – the `.ho` header `VERSION_MAJOR`/`VERSION_MINOR`,
     kept in lock-step with the two leading version components (the patch
     component is documentation-only and is not encoded in the header).
4. Create a git commit: `chore: bump version to X.Y.Z [skip ci]`
5. Create a git tag: `vX.Y.Z`

## GitFlow Workflow

The repository follows [GitFlow](https://nvie.com/posts/a-successful-git-branching-model/):

| Branch        | Purpose | Branched from | Merges into |
|---------------|---------|---------------|-------------|
| `main`        | Stable / production (protected) | – | – |
| `dev`         | Integration / development       | `main` | `main` |
| `feature/*`   | New features                    | `dev` | `dev` |
| `release/*`   | Preparing a release             | `dev` | `main` and `dev` |
| `hotfix/*`    | Urgent fixes to production      | `main` | `main` and `dev` |

Releases are produced **only from `main`**. The `dev` branch is the
development/integration line and is never tagged or released directly; after
every release CI syncs `main` back into `dev`.

Releasing a new version (from the `1.0.0` base, a `release/*` merge bumps the
minor to `1.1.0`; a `hotfix/*` merge bumps the patch to `1.0.1`; a `BREAKING`
change bumps the major):

```bash
# 1. Cut a release branch from dev
git checkout dev
git checkout -b release/v1.1.0
# 2. Final tweaks, then merge back into main (bumps minor) and dev
git checkout main && git merge --no-ff release/v1.1.0
git checkout dev  && git merge --no-ff release/v1.1.0
```

CI detects the `release/*` or `hotfix/*` source from the merge commit, bumps the
version, tags `vX.Y.Z` on `main`, and syncs the version back into `dev`. The tag
is pushed to `main`, and only a `main`-contained tag can trigger a GitHub
Release (see `create-release` below).

## Generating Release Notes Manually

```bash
python scripts/generate_changelog.py v0.2.0          # auto-detect previous tag
python scripts/generate_changelog.py v0.2.0 v0.1.0   # explicit range
```

This outputs Markdown to stdout. Redirect to a file:
```bash
python scripts/generate_changelog.py v0.2.0 > RELEASE_NOTES.md
```

## CI/CD Pipelines

All jobs live in `.github/workflows/build-and-test.yml`:

| Job | Trigger |
|-----|---------|
| **build-macos** (Apple Silicon) | Push / PR to `main`, `dev`, `feature/*`, `release/*`, `hotfix/*`; tag push `v*`; skipped on `[skip ci]` autopushes |
| **build-linux** (x64, Release) | Push / PR to `main`, `dev`, `feature/*`, `release/*`, `hotfix/*`; tag push `v*`; skipped on `[skip ci]` autopushes |
| **build-windows** (x64, Release) | Push / PR to `main`, `dev`, `feature/*`, `release/*`, `hotfix/*`; tag push `v*`; skipped on `[skip ci]` autopushes |
| **create-release-bundle** | Push to `main` (after the three build jobs; skipped on `[skip ci]`) |
| **bump-version** | Push to `main` from a `release/*` or `hotfix/*` merge, or an admin `workflow_dispatch` with a `bump_mode` override (after the three build jobs) |
| **sync-main-to-dev** | Push to `main` (after the three build jobs and `bump-version`; merges the new version back into `dev`) |
| **create-release** | `v*` tag push; verifies the tag is contained in `main`, so releases happen only from `main` |

### Linux Pipeline Notes
- Runs on `ubuntu-latest` (Ubuntu 24.04) with LLVM 22 (downloaded from the LLVM
  release asset), Ninja, and CMake; libuv/ssl/curl/zip/zstd/nlohmann-json come
  from apt, and ANTLR4's C++ runtime is built from source via CMake FetchContent
  (not vcpkg). GoogleTest comes from the distro's prebuilt `libgtest-dev`
  package (found directly by `find_package(GTest)` — no from-source compile).
- Configures a single `Release` build and runs `ctest` after the binary check.
- Uploads `hoo-linux-x86_64.tar.gz` as an artifact.

The three build jobs share a `[skip ci]` guard: when a version bump is pushed
back to `main`, `bump_version.py` commits with a `[skip ci]` message, so the
autopush does not trigger a redundant full cross-platform rebuild. Presubmit
(PR) runs always build.

### Windows Pipeline Notes
- Runs on `windows-latest` with the MSVC toolchain and LLVM 22.
- Uses the `windows-vs18-env` CMake preset; dependencies (ANTLR4, curl, etc.)
  come from vcpkg.
- Runs `ctest` and packages `hoo-windows-x64.zip`.

### Release Pipeline Notes
- On a push to `main` the macOS, Linux, and Windows builds run first, then a
  combined `hoo-all-platforms.tar.gz` bundle is assembled.
- The **bump-version** job inspects the merge commit message to detect whether
  the change came from a `release/*` (minor) or `hotfix/*` (patch) branch and
  bumps/tags accordingly. An admin `workflow_dispatch` with a `bump_mode`
  override can force the level (main only). If the source merge cannot be
  identified and no override is given, the bump is skipped (never guessed).
- The **sync-main-to-dev** job then merges `main` into `dev` so the development
  branch carries the released version as its new base.
- On a tag push matching `v*`, a GitHub Release is published with the platform
  binaries attached and a categorised changelog generated from git history.
  Because tags are created only by **bump-version** on `main`, and
  **create-release** verifies the tag is contained in `origin/main`, releases
  can only ever be produced from `main` — there is no manual tag-release path.
- Versioning is based on the `1.0.0` release with standard pre-1.0/1.x SemVer:
  `release/*` merges bump the minor, `hotfix/*` merges bump the patch, and a
  `BREAKING CHANGE` escalates to a major bump. The `.ho` module header
  (`HOModule.h` `VERSION_MAJOR`/`VERSION_MINOR`) tracks the two leading
  components; the patch is documentation-only. `dev` carries the in-development
  version and is synced from `main` after each release.

## Conventional Commit Format

Follow the [Conventional Commits](https://www.conventionalcommits.org/) specification:

```
<type>(<optional scope>): <short summary>

[optional body]

[optional footer: BREAKING CHANGE: ...]
```

**Types:** `feat`, `fix`, `chore`, `refactor`, `docs`, `test`, `perf`, `build`, `ci`

**Examples:**
```
feat(parser): add support for async fn keyword
fix(jit): correct namespace qualification for HVMJIT methods
BREAKING: rename hoo_readchar return type to HooCharacter
chore: bump version to 0.2.0
```
