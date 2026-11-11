#!/usr/bin/env python3
#
# How it works:
#   1. find the last N+1 commits that touched src/version.h.in (the build bumps)
#   2. for each adjacent pair (previous, current), read PROJECT_VERSION_BUILD_NO
#      from the file at both commits -> e.g. 483 -> 484
#   3. list every commit in (previous, current], dropping the ones that only bump
#      the build number / version (i.e. touch nothing but src/version.h.in)
#   4. emit N such blocks, newest build first. Build numbers are the values
#      actually recorded at each bump commit, so a skipped build stays skipped
#      (e.g. 503 -> 504 then 501 -> 503 if 502 was never committed).
#
# Output (default --html, a single line, blocks separated by <br><br>):
#   Changes for build 484 -&gt; 485:<br>#1&nbsp;...<br><br>Changes for build 483 -&gt; 484:<br>...
#
# Usage:
#   python build_changelog.py [REPO_DIR] [-n N] [--plain]
#   REPO_DIR defaults to the parent of this script's directory; N defaults to 3

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


def commits_between(repo, prev, cur):
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
    return commits


def collect(repo, n):
    # n+1 most recent commits that touched the version file -> n adjacent pairs, newest first
    bumps = git(repo, "log", "-n", str(n + 1), "--format=%H", "--", VERSION_FILE).split()
    segments = []
    for cur, prev in zip(bumps, bumps[1:]):
        segments.append({
            "old": build_no_at(repo, prev),
            "new": build_no_at(repo, cur),
            "commits": commits_between(repo, prev, cur),
        })
    return segments


def my_html_escape(s):
    # % -> &#37; to keep the .bat capture safe
    return html.escape(s).replace("%", "&#37;")


def render_html(segments):
    blocks = []
    for seg in segments:
        parts = [f"Changes for build {my_html_escape(seg['old'])} -&gt; {my_html_escape(seg['new'])}:"]
        if not seg["commits"]:
            parts.append("(no changes)")
        for i, c in enumerate(seg["commits"], 1):
            parts.append(f"#{i}&nbsp;&nbsp;{c['hash']} by {my_html_escape(c['author'])}, {c['date']}")
            parts.append(f"&nbsp;&nbsp;&nbsp;&nbsp;&quot;{my_html_escape(c['subject'])}&quot;")    # &quot; (not a raw ") so the line is safe to capture with: for /f ... do set "var=%%i"
        blocks.append("<br>".join(parts))
    return "<br><br>".join(blocks)  # single physical line, no raw double-quotes


def render_plain(segments):
    blocks = []
    for seg in segments:
        lines = [f"Changes for build {seg['old']} -> {seg['new']}:"]
        if not seg["commits"]:
            lines.append("(no changes)")
        for i, c in enumerate(seg["commits"], 1):
            lines.append(f"#{i}  {c['hash']} by {c['author']}, {c['date']}")
            lines.append(f"    \"{c['subject']}\"")
        blocks.append("\n".join(lines))
    return "\n\n".join(blocks)


def main():
    # emit UTF-8 regardless of the platform codepage (commit subjects may be non-ASCII);
    # otherwise a redirected stdout on Windows raises UnicodeEncodeError -> "(unavailable)"
    try:
        sys.stdout.reconfigure(encoding="utf-8")
    except (AttributeError, ValueError):
        pass

    args = [a for a in sys.argv[1:]]
    plain = "--plain" in args
    args = [a for a in args if a != "--plain"]

    count = 3
    if "-n" in args:
        i = args.index("-n")
        count = int(args[i + 1])
        del args[i:i + 2]

    repo = args[0] if args else os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

    try:
        segments = collect(repo, count)
        if not segments:
            # not enough history to compare; keep the e-mail intact
            print("Changes: (build history unavailable)")
            return 0
        print(render_plain(segments) if plain else render_html(segments))
    except Exception as e:
        # never break the build notification because of the changelog
        sys.stderr.write(f"build_changelog: {e}\n")
        print("Changes: (unavailable)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
