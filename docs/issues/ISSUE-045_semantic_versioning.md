# ISSUE-045 Semantic Versioning, Version Bumping, Linux Build Pipelines & GitHub Release Workflow

## Overview
This issue tracks the implementation of a **robust semantic versioning** strategy for the Hoo project, automated **version bumping**, a **Linux CI/CD pipeline**, and a **GitHub Release workflow** that publishes artifacts (binaries, docs, and changelog) for each tagged version.

---

## Motivation
* **Predictable releases** – Consumers need to know whether a new release contains breaking changes, new features, or bug fixes.
* **Automation** – Manual version updates are error‑prone. An automated bump reduces human overhead and ensures consistency between `README`, `CMakeLists.txt`, package manifests, and Git tags.
* **Cross‑platform CI** – The project currently builds on macOS only. Supporting Linux (the most common CI environment) expands the contributor base and allows us to publish Linux binaries.
* **GitHub Release automation** – Publishing compiled binaries, documentation PDFs, and a generated changelog directly from CI improves discoverability and accelerates adoption.

---

## Design Goals
1. Adopt **Semantic Versioning 2.0.0** (`MAJOR.MINOR.PATCH`).
2. Provide **conventional commit** parsing to decide the bump type automatically.
3. Implement a **`bump_version`** script that updates:
   - `CMakeLists.txt` (project version variables).
   - `docs/CHANGELOG.md` (new version heading).
   - `README.md` badge (version badge URL).
   - Any package manager manifest (e.g., `package.json` if Node bindings exist).
4. Add a **Linux CI job** using GitHub Actions that:
   - Checks out code, sets up a Linux build environment (Ubuntu‑latest, Ninja, Homebrew on Linux, or apt packages).
   - Runs `cmake` and `ninja` to produce `hoo` binaries.
   - Executes the full test suite.
5. Extend the **GitHub Release workflow** to:
   - Trigger on a new Git tag (`v*`).
   - Build Linux and macOS binaries (using matrix strategy).
   - Upload artifacts (`hoo-linux.tar.gz`, `hoo-macos.tar.gz`).
   - Generate a changelog section from merged PR titles (using `github-changelog-generator` or a custom script).
   - Publish a release with the generated notes.

---

## Implementation Steps
### 1. Semantic Versioning & Bump Script
- Create `scripts/bump_version.py` (Python) or `scripts/bump_version.sh` (bash).
- Use the `git log` with `--grep` to detect `feat:`, `fix:`, `BREAKING CHANGE:` prefixes.
- Determine bump level: **major** for breaking changes, **minor** for new features, **patch** for bug fixes and chores.
- Update version strings in:
  - `CMakeLists.txt` (`project(hoo VERSION X.Y.Z)`).
  - `docs/CHANGELOG.md` (prepend a new heading `## X.Y.Z – YYYY‑MM‑DD`).
  - `README.md` (replace version badge URL: `https://img.shields.io/badge/version-X.Y.Z-blue`).
- Commit the changes with a conventional commit message `chore: bump version to X.Y.Z`.
- Tag the commit with `git tag vX.Y.Z`.

### 2. Linux Build Pipeline (GitHub Actions)
Create `.github/workflows/linux-build.yml`:
```yaml
name: Linux Build & Test
on: [push, pull_request]
jobs:
  build-linux:
    runs-on: ubuntu-latest
    strategy:
      matrix:
        build-type: [Debug, Release]
    steps:
      - uses: actions/checkout@v4
      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y build-essential ninja-build cmake libuv1-dev
      - name: Configure
        run: cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=${{ matrix.build-type }}
      - name: Build
        run: cmake --build build --target all
      - name: Run tests
        run: ./build/hoo-tests
      - name: Archive binaries
        if: matrix.build-type == 'Release'
        run: |
          tar czf hoo-linux-${{ matrix.build-type }}.tar.gz -C build hoo
        # The artifact will be used by the release workflow.
```

### 3. GitHub Release Workflow
Create `.github/workflows/release.yml`:
```yaml
name: Release
on:
  push:
    tags:
      - 'v*'
jobs:
  build:
    runs-on: ${{ matrix.os }}
    strategy:
      matrix:
        os: [ubuntu-latest, macos-latest]
        include:
          - os: ubuntu-latest
            artifact: hoo-linux.tar.gz
          - os: macos-latest
            artifact: hoo-macos.tar.gz
    steps:
      - uses: actions/checkout@v4
      - name: Setup build environment
        if: runner.os == 'Linux'
        run: sudo apt-get update && sudo apt-get install -y build-essential ninja-build cmake libuv1-dev
      - name: Setup build environment (macOS)
        if: runner.os == 'macOS'
        run: brew install ninja cmake libuv
      - name: Configure
        run: cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
      - name: Build
        run: cmake --build build --target all
      - name: Package
        run: |
          tar czf ${{ matrix.artifact }} -C build hoo
      - name: Upload artifact
        uses: actions/upload-artifact@v4
        with:
          name: ${{ matrix.artifact }}
          path: ${{ matrix.artifact }}
  create-release:
    needs: build
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Download artifacts
        uses: actions/download-artifact@v4
        with:
          path: ./artifacts
      - name: Generate changelog
        run: |
          python scripts/generate_changelog.py ${{ github.ref_name }} > RELEASE_NOTES.md
      - name: Create GitHub Release
        uses: softprops/action-gh-release@v2
        with:
          tag_name: ${{ github.ref_name }}
          name: Release ${{ github.ref_name }}
          body_path: RELEASE_NOTES.md
          files: ./artifacts/*
```
- The `generate_changelog.py` script reads merged PR titles between the previous tag and the current tag, formats them, and writes `RELEASE_NOTES.md`.

### 4. Documentation
- Add a new section to `docs/DEVELOPMENT.md` explaining how to run `scripts/bump_version.py` locally.
- Update `README.md` badge to reference the generated GitHub release tag.
- Document the CI status badges (`Linux Build`, `macOS Build`) in the top of the README.

---

## Risks & Mitigations
| Risk | Impact | Mitigation |
|------|--------|------------|
| Incorrect version bump due to malformed commit messages | Wrong version released, possible breaking‑change confusion | Enforce commit‑message linting with `commitlint` in PR checks. |
| Linux CI environment differs from developer machines | Build failures that are hard to reproduce | Pin exact versions of compiler, cmake, and libuv; provide a Dockerfile for reproducible local builds. |
| Release workflow publishes broken artifacts | Users download non‑functional binaries | Add an extra validation step in the release job that runs `./hoo --version` on the packaged binary before uploading. |
| Changelog generation misses entries | Incomplete release notes | Run `git fetch --tags --prune` before generating the changelog; fallback to `git log --oneline` if script fails. |

---

## Acceptance Criteria
1. **Semantic version bump** – Running `scripts/bump_version.py` updates all version locations, commits, tags, and passes the conventional‑commit checks.
2. **Linux CI** – A push to any branch triggers the Linux build job, which compiles, runs the full test suite, and archives release binaries.
3. **GitHub Release** – Creating a tag `vX.Y.Z` automatically produces a GitHub Release with both Linux and macOS binary assets and a correctly generated changelog.
4. **Documentation** – All README badges, version references, and developer guides reflect the new versioning workflow.
5. **No regressions** – Existing macOS pipeline remains untouched and continues to publish macOS artifacts as before.

---

*Prepared by Antigravity AI – 2026‑06‑21*
