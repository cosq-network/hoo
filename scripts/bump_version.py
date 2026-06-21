#!/usr/bin/env python3
"""
bump_version.py – Semantic version bumping for the Hoo project.

Usage:
    python scripts/bump_version.py [major|minor|patch|auto]

    auto  – inspect git log since last tag and pick the bump level from
            conventional commit prefixes:
              BREAKING CHANGE or 'BREAKING:' → major
              feat:                           → minor
              fix: / chore: / refactor: etc  → patch

The script updates:
  - CMakeLists.txt  (project version)
  - docs/CHANGELOG.md (prepend new section)
  - README.md (version badge)
Then commits and tags.
"""

import subprocess
import sys
import re
import datetime
import os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


# ---------------------------------------------------------------------------
# Git helpers
# ---------------------------------------------------------------------------

def git(*args):
    result = subprocess.run(["git"] + list(args), capture_output=True, text=True, cwd=ROOT)
    if result.returncode != 0:
        print(f"[git error] {result.stderr.strip()}", file=sys.stderr)
        sys.exit(1)
    return result.stdout.strip()


def last_tag():
    try:
        return git("describe", "--tags", "--abbrev=0")
    except SystemExit:
        return None


def commits_since(tag):
    if tag:
        return git("log", f"{tag}..HEAD", "--pretty=format:%s").splitlines()
    return git("log", "--pretty=format:%s").splitlines()


# ---------------------------------------------------------------------------
# Version helpers
# ---------------------------------------------------------------------------

def read_current_version():
    cmake = os.path.join(ROOT, "CMakeLists.txt")
    with open(cmake) as f:
        content = f.read()
    m = re.search(r'project\s*\(.*?VERSION\s+(\d+)\.(\d+)\.(\d+)', content, re.DOTALL)
    if m:
        return int(m.group(1)), int(m.group(2)), int(m.group(3))
    # Fallback: look for standalone set(HOO_VERSION ...)
    m = re.search(r'set\s*\(\s*HOO_VERSION\s+"?(\d+)\.(\d+)\.(\d+)"?\s*\)', content)
    if m:
        return int(m.group(1)), int(m.group(2)), int(m.group(3))
    return 0, 1, 0  # default starting version


def bump_level_from_commits(commits):
    for msg in commits:
        if "BREAKING CHANGE" in msg or msg.startswith("BREAKING:"):
            return "major"
    for msg in commits:
        if msg.startswith("feat"):
            return "minor"
    return "patch"


def calc_new_version(major, minor, patch, level):
    if level == "major":
        return major + 1, 0, 0
    if level == "minor":
        return major, minor + 1, 0
    return major, minor, patch + 1


# ---------------------------------------------------------------------------
# File updaters
# ---------------------------------------------------------------------------

def update_cmake(old_ver, new_ver):
    cmake = os.path.join(ROOT, "CMakeLists.txt")
    with open(cmake) as f:
        content = f.read()

    old_str = ".".join(str(x) for x in old_ver)
    new_str = ".".join(str(x) for x in new_ver)

    # Replace inside project(...VERSION X.Y.Z...) first
    updated = re.sub(
        r'(project\s*\([^)]*?VERSION\s+)(\d+\.\d+\.\d+)',
        lambda m: m.group(1) + new_str,
        content,
        flags=re.DOTALL
    )
    # Also replace any explicit HOO_VERSION variable
    updated = re.sub(
        r'(set\s*\(\s*HOO_VERSION\s+"?)(\d+\.\d+\.\d+)',
        lambda m: m.group(1) + new_str,
        updated
    )

    if updated == content:
        print(f"[warn] CMakeLists.txt: version {old_str} not found; adding HOO_VERSION variable.")
        updated = content.rstrip() + f'\nset(HOO_VERSION "{new_str}")\n'

    with open(cmake, "w") as f:
        f.write(updated)
    print(f"  CMakeLists.txt  {old_str} → {new_str}")


def update_changelog(new_ver, commits):
    changelog = os.path.join(ROOT, "docs", "CHANGELOG.md")
    ver_str = ".".join(str(x) for x in new_ver)
    today = datetime.date.today().isoformat()
    header = f"## {ver_str} – {today}\n\n"
    bullet_lines = "\n".join(f"- {c}" for c in commits if c.strip()) if commits else "- Version bump."
    new_section = header + bullet_lines + "\n\n"

    if os.path.exists(changelog):
        with open(changelog) as f:
            existing = f.read()
        # Don't duplicate
        if f"## {ver_str}" in existing:
            print(f"  CHANGELOG.md    already has section {ver_str}; skipping.")
            return
        # Insert after first heading line
        lines = existing.splitlines(keepends=True)
        insert_at = 0
        for i, line in enumerate(lines):
            if line.startswith("## "):
                insert_at = i
                break
        lines.insert(insert_at, new_section)
        updated = "".join(lines)
    else:
        updated = f"# Changelog\n\n{new_section}"

    with open(changelog, "w") as f:
        f.write(updated)
    print(f"  CHANGELOG.md    prepended section {ver_str}")


def update_readme(new_ver):
    readme = os.path.join(ROOT, "README.md")
    ver_str = ".".join(str(x) for x in new_ver)
    with open(readme) as f:
        content = f.read()

    # Replace existing shields.io version badge
    updated, n = re.subn(
        r'https://img\.shields\.io/badge/version-[\d.]+-[a-zA-Z]+',
        f'https://img.shields.io/badge/version-{ver_str}-blue',
        content
    )
    if n == 0:
        # Insert badge at the top after the title line
        lines = updated.splitlines(keepends=True)
        badge = f'[![Version](https://img.shields.io/badge/version-{ver_str}-blue)](https://github.com/cosq-network/hoo/releases/tag/v{ver_str})\n'
        for i, line in enumerate(lines):
            if line.startswith("# "):
                lines.insert(i + 1, badge)
                break
        updated = "".join(lines)
        print(f"  README.md       inserted version badge {ver_str}")
    else:
        print(f"  README.md       updated version badge → {ver_str}")

    with open(readme, "w") as f:
        f.write(updated)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    level_arg = sys.argv[1] if len(sys.argv) > 1 else "auto"
    if level_arg not in ("major", "minor", "patch", "auto"):
        print(f"Usage: bump_version.py [major|minor|patch|auto]")
        sys.exit(1)

    old_ver = read_current_version()
    tag = last_tag()
    commits = commits_since(tag)

    level = bump_level_from_commits(commits) if level_arg == "auto" else level_arg
    new_ver = calc_new_version(*old_ver, level)
    ver_str = ".".join(str(x) for x in new_ver)

    print(f"\nBumping version: {'.'.join(str(x) for x in old_ver)} → {ver_str} ({level})\n")

    update_cmake(old_ver, new_ver)
    update_changelog(new_ver, commits)
    update_readme(new_ver)

    # Stage and commit
    git("add", "CMakeLists.txt", "docs/CHANGELOG.md", "README.md")
    git("commit", "-m", f"chore: bump version to {ver_str}")
    git("tag", f"v{ver_str}")

    print(f"\n✓ Version bumped to {ver_str} and tagged as v{ver_str}")
    print(f"  Push with: git push origin main --tags")


if __name__ == "__main__":
    main()
