# Development Guide

## Versioning

This project uses **Semantic Versioning 2.0.0** (`MAJOR.MINOR.PATCH`):

| Bump | Trigger |
|------|---------|
| **MAJOR** | Commit messages containing `BREAKING CHANGE:` or starting with `BREAKING:` |
| **MINOR** | Commit messages starting with `feat:` |
| **PATCH** | Any other commit (`fix:`, `chore:`, `refactor:`, `docs:`, etc.) |

## Bumping the Version

Run the bump script from the project root:

```bash
# Automatically detect bump level from git log since last tag
python scripts/bump_version.py auto

# Or specify explicitly
python scripts/bump_version.py major
python scripts/bump_version.py minor
python scripts/bump_version.py patch
```

The script will:
1. Read the current version from `CMakeLists.txt`.
2. Scan git log since the last tag to determine the bump level (when using `auto`).
3. Update:
   - `CMakeLists.txt` – `project(Hoo VERSION X.Y.Z)`
   - `docs/CHANGELOG.md` – prepend a new version section with commit messages
   - `README.md` – update the version badge URL
4. Create a git commit: `chore: bump version to X.Y.Z`
5. Create a git tag: `vX.Y.Z`

Push to GitHub to trigger the release workflow:
```bash
git push origin main --tags
```

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
| **build-macos** (Apple Silicon) | Push / PR to `main`, `develop`; tag push `v*` |
| **build-linux** (x64, Release) | Push / PR to `main`, `develop`; tag push `v*` |
| **build-windows** (x64, Release) | Push / PR to `main`, `develop`; tag push `v*` |
| **create-release-bundle** | Push to `main` (after the three build jobs) |
| **bump-version** | Push to `main` (after the three build jobs) |
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
  combined `hoo-all-platforms.tar.gz` bundle is assembled and the version is
  bumped automatically.
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
