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
4. Create a git commit: `chore: bump version to X.Y.Z`
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

Releasing a new version:

```bash
# 1. Cut a release branch from dev
git checkout dev
git checkout -b release/v1.5.0
# 2. Final tweaks, then merge back into main (bumps minor) and dev
git checkout main && git merge --no-ff release/v1.5.0
git checkout dev  && git merge --no-ff release/v1.5.0
```

CI detects the `release/*` or `hotfix/*` source from the merge commit, bumps the
version, tags `vX.Y.Z` on `main`, and syncs the version back into `dev`.

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
| **build-macos** (Apple Silicon) | Push / PR to `main`, `dev`, `feature/*`, `release/*`, `hotfix/*`; tag push `v*` |
| **build-linux** (x64, Release) | Push / PR to `main`, `dev`, `feature/*`, `release/*`, `hotfix/*`; tag push `v*` |
| **build-windows** (x64, Release) | Push / PR to `main`, `dev`, `feature/*`, `release/*`, `hotfix/*`; tag push `v*` |
| **create-release-bundle** | Push to `main` (after the three build jobs) |
| **bump-version** | Push to `main` from a `release/*` or `hotfix/*` merge (after the three build jobs) |
| **sync-main-to-dev** | Push to `main` (merges the new version back into `dev`) |
| **create-release** | Tag push `v*` or manual dispatch |

### Linux Pipeline Notes
- Runs on `ubuntu-latest` with LLVM 22 (downloaded from the LLVM release asset),
  Ninja, CMake, ANTLR4 (via vcpkg), GoogleTest, and libuv/ssl/curl/zip/zstd
  installed from apt.
- Configures a single `Release` build and runs `ctest` after the binary check.
- Uploads `hoo-linux-x86_64.tar.gz` as an artifact.

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
  bumps/tags accordingly. A manual `workflow_dispatch` `bump_mode` input can
  override detection. If the source merge cannot be identified, the bump is
  skipped (never guessed).
- The **sync-main-to-dev** job then merges `main` into `dev` so the development
  branch carries the released version as its new base.
- On tag pushes matching `v*` (or a manual `workflow_dispatch`), a GitHub
  Release is published with the platform binaries attached and a categorised
  changelog generated from git history.

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
