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

| Workflow | File | Trigger |
|----------|------|---------|
| **macOS Build & Test** | `.github/workflows/build-and-test.yml` | Push / PR to `main`, `develop` |
| **Linux Build & Test** | `.github/workflows/linux-build.yml` | Push / PR to `main`, `develop` |
| **GitHub Release** | `.github/workflows/release.yml` | Tag push `v*` or manual dispatch |

### Linux Pipeline Notes
- Runs on `ubuntu-latest` with LLVM 22, Ninja, CMake, ANTLR4, GoogleTest, and libuv installed from apt.
- Builds both `Debug` and `Release` configurations.
- Uploads `hoo-linux-x86_64.tar.gz` artifact for Release builds.

### GitHub Release Notes
- Triggered automatically on tag pushes matching `v*`.
- Builds Linux and macOS binaries in parallel.
- Validates each binary (`./hoo --version`) before packaging.
- Generates a categorised changelog from git history.
- Publishes a GitHub Release with attached `.tar.gz` assets and the generated changelog.

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
