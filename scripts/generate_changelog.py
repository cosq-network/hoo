#!/usr/bin/env python3
"""
generate_changelog.py – Generate release notes between two Git tags.

Usage:
    python scripts/generate_changelog.py <new_tag> [<old_tag>]

If <old_tag> is omitted the script uses the tag that precedes <new_tag>.
Output is printed to stdout (redirect to RELEASE_NOTES.md in CI).
"""

import subprocess
import sys
import re
import os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def git(*args):
    result = subprocess.run(["git"] + list(args), capture_output=True, text=True, cwd=ROOT)
    return result.stdout.strip()


def all_tags():
    raw = git("tag", "--sort=-version:refname")
    return [t for t in raw.splitlines() if t.strip()]


def previous_tag(current_tag):
    tags = all_tags()
    try:
        idx = tags.index(current_tag)
        return tags[idx + 1] if idx + 1 < len(tags) else None
    except ValueError:
        return None


def commits_between(old_tag, new_tag):
    ref_range = f"{old_tag}..{new_tag}" if old_tag else new_tag
    raw = git("log", ref_range, "--pretty=format:%H|%s|%an")
    entries = []
    for line in raw.splitlines():
        if not line.strip():
            continue
        parts = line.split("|", 2)
        sha = parts[0][:7] if len(parts) > 0 else "?"
        subject = parts[1] if len(parts) > 1 else line
        author = parts[2] if len(parts) > 2 else ""
        entries.append((sha, subject, author))
    return entries


CATEGORIES = [
    ("breaking", re.compile(r'^(BREAKING CHANGE|BREAKING):'), "💥 Breaking Changes"),
    ("feat",     re.compile(r'^feat(\(.+?\))?:'),             "✨ New Features"),
    ("fix",      re.compile(r'^fix(\(.+?\))?:'),              "🐛 Bug Fixes"),
    ("perf",     re.compile(r'^perf(\(.+?\))?:'),             "⚡ Performance"),
    ("refactor", re.compile(r'^refactor(\(.+?\))?:'),         "♻️ Refactoring"),
    ("docs",     re.compile(r'^docs(\(.+?\))?:'),             "📝 Documentation"),
    ("test",     re.compile(r'^test(\(.+?\))?:'),             "🧪 Tests"),
    ("chore",    re.compile(r'^chore(\(.+?\))?:'),            "🔧 Chores"),
]

REPO_URL = "https://github.com/cosq-network/hoo"


def categorize(entries):
    buckets = {k: [] for k, _, _ in CATEGORIES}
    buckets["other"] = []
    for sha, subject, author in entries:
        matched = False
        for key, pattern, _ in CATEGORIES:
            if pattern.match(subject):
                buckets[key].append((sha, subject, author))
                matched = True
                break
        if not matched:
            buckets["other"].append((sha, subject, author))
    return buckets


def format_entry(sha, subject, author):
    link = f"[`{sha}`]({REPO_URL}/commit/{sha})"
    return f"- {subject} ({link}, @{author})"


def generate(new_tag, old_tag):
    entries = commits_between(old_tag, new_tag)
    buckets = categorize(entries)

    lines = []
    lines.append(f"# Release {new_tag}\n")
    if old_tag:
        lines.append(f"**Full diff:** [{old_tag}...{new_tag}]({REPO_URL}/compare/{old_tag}...{new_tag})\n")
    lines.append("")

    for key, _, label in CATEGORIES:
        items = buckets.get(key, [])
        if items:
            lines.append(f"## {label}\n")
            for sha, subject, author in items:
                lines.append(format_entry(sha, subject, author))
            lines.append("")

    others = buckets.get("other", [])
    if others:
        lines.append("## 🗂 Other Changes\n")
        for sha, subject, author in others:
            lines.append(format_entry(sha, subject, author))
        lines.append("")

    if not entries:
        lines.append("_No changes found between tags._\n")

    return "\n".join(lines)


def main():
    if len(sys.argv) < 2:
        print("Usage: generate_changelog.py <new_tag> [<old_tag>]", file=sys.stderr)
        sys.exit(1)
    new_tag = sys.argv[1]
    old_tag = sys.argv[2] if len(sys.argv) > 2 else previous_tag(new_tag)
    print(generate(new_tag, old_tag))


if __name__ == "__main__":
    main()
