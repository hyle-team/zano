#!/usr/bin/env python3
#
# How it works:
#   1. find the latest commit that touched src/version.h.in        -> "current" build
#   2. find the commit that touched it right before that one       -> "previous" build
#   3. read PROJECT_VERSION_BUILD_NO from the file at both commits  -> e.g. 483 -> 484
#   4. list every commit in (previous, current], dropping the ones that only bump
#      the build number / version (i.e. touch nothing but src/version.h.in)
#
# Output (default --html, a single line):
#   Changes for build 483 -&gt; 484:<br>#1&nbsp;&nbsp;0abfc18d by sowle, 2026-06-04 ...<br>...
#
# Usage:
#   python build_changelog.py [REPO_DIR] [--plain]
#   REPO_DIR defaults to the parent of this script's directory

import sys
import os
import re
import subprocess
import html

VERSION_FILE = "src/version.h.in"
BUILD_NO_RE = re.compile(r"^#define\s+PROJECT_VERSION_BUILD_NO\s+(\d+)", re.MULTILINE)


def git(repo, *args):
    out = subprocess.check_output(["git", "-C", repo, *args], stderr=subprocess.STDOUT)
    return out.decode("utf-8", "replace")


def build_no_at(repo, commit):
    text = git(repo, "show", f"{commit}:{VERSION_FILE}")
    m = BUILD_NO_RE.search(text)
    return m.group(1) if m else "?"


def changed_files(repo, commit):
    out = git(repo, "show", "--pretty=format:", "--name-only", commit)
    return [ln.strip() for ln in out.splitlines() if ln.strip()]


def collect(repo):
    # two most recent commits that touched the version file
    bumps = git(repo, "log", "-n", "2", "--format=%H", "--", VERSION_FILE).split()
    if len(bumps) < 2:
        return None, None, []
    cur, prev = bumps[0], bumps[1]

    sep = "\x1f"
    fmt = sep.join(["%H", "%an", "%ad", "%s"])
    log = git(repo, "log", f"--format={fmt}",                              #  "--no-merges",
              "--date=format:%Y-%m-%d %H:%M:%S", f"{prev}..{cur}")

    commits = []
    for line in log.splitlines():
        if not line:
            continue
        full_hash, author, date, subject = line.split(sep, 3)
        # drop commits that only change the build number / version
        if changed_files(repo, full_hash) == [VERSION_FILE]:
            continue
        commits.append({
            "hash": full_hash[:8],
            "author": author,
            "date": date,
            "subject": subject.splitlines()[0] if subject else "",  # first line only
        })
    return build_no_at(repo, prev), build_no_at(repo, cur), commits


def my_html_escape(s):
    # % -> &#37; to keep the .bat capture safe
    return html.escape(s).replace("%", "&#37;")


def render_html(old, new, commits):
    parts = [f"Changes for build {my_html_escape(old)} -&gt; {my_html_escape(new)}:"]
    if not commits:
        parts.append("(no changes)")
    for i, c in enumerate(commits, 1):
        parts.append(f"#{i}&nbsp;&nbsp;{c['hash']} by {my_html_escape(c['author'])}, {c['date']}")
        parts.append(f"&nbsp;&nbsp;&nbsp;&nbsp;&quot;{my_html_escape(c['subject'])}&quot;")    # &quot; (not a raw ") so the line is safe to capture with: for /f ... do set "var=%%i"
    return "<br>".join(parts)  # single physical line, no raw double-quotes


def render_plain(old, new, commits):
    lines = [f"Changes for build {old} -> {new}:"]
    if not commits:
        lines.append("(no changes)")
    for i, c in enumerate(commits, 1):
        lines.append(f"#{i}  {c['hash']} by {c['author']}, {c['date']}")
        lines.append(f"    \"{c['subject']}\"")
    return "\n".join(lines)


def main():
    args = [a for a in sys.argv[1:]]
    plain = "--plain" in args
    args = [a for a in args if a != "--plain"]
    repo = args[0] if args else os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

    try:
        old, new, commits = collect(repo)
        if old is None:
            # not enough history to compare; keep the e-mail intact
            print("Changes: (build history unavailable)")
            return 0
        print(render_plain(old, new, commits) if plain else render_html(old, new, commits))
    except Exception as e:
        # never break the build notification because of the changelog
        sys.stderr.write(f"build_changelog: {e}\n")
        print("Changes: (unavailable)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
