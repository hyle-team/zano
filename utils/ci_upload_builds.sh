#!/usr/bin/env bash
#
# CI publish helper. The per-platform workflows already name their artifacts with
# the public download convention:
#   zano-<os>-<arch>-<kind>-<branch>[-testnet]-v<ver>[<shortsha>]   (os: win|macos|linux)
# The publish job scopes actions/download-artifact to this network already, so
# this script copies each file out under "<artifact-name>.<ext>", uploads them to
# the builds server over SSH, and prints a manifest (URL + sha256) for signing.
#
# Reused by publish-mainnet / publish-testnet in build-all.yml; they differ only
# by NETWORK (mainnet|testnet).
#
# Required env:
#   NETWORK      mainnet | testnet      mainnet skips "-testnet-" artifacts; testnet takes only them
#   DL_DIR       download-artifact target dir
#   SSH_TARGET   [user@]host            (secret)
#   SSH_KEY      private key contents   (secret)
# Optional env:
#   SSH_PORT       ssh port (default 22)
#   KNOWN_HOSTS    known_hosts contents; when set the host key is pinned
#   UPLOAD_RETRIES scp attempts before giving up (default 3)
#   OUT_DIR        staging dir (default ./out)
#   URL_BASE       default https://build.zano.org/builds
#   REMOTE_DIR     default /var/www/html/builds

set -euo pipefail

: "${NETWORK:?NETWORK is required}"
: "${DL_DIR:?DL_DIR is required}"
OUT_DIR="${OUT_DIR:-out}"
URL_BASE="${URL_BASE:-https://build.zano.org/builds}"
REMOTE_DIR="${REMOTE_DIR:-/var/www/html/builds}"
mkdir -p "$OUT_DIR"

ext_of() {
  case "$1" in
    *.tar.bz2) echo "tar.bz2" ;;
    *.AppImage) echo "AppImage" ;;
    *.dmg) echo "dmg" ;;
    *.zip) echo "zip" ;;
    *.exe) echo "exe" ;;
    *) echo "${1##*.}" ;;
  esac
}

# --- select this network's artifacts, copy under their public name, hash --------
# download-artifact is already scoped per network; the "-testnet-" guard below is
# a belt-and-suspenders second check.
declare -a WIN=() MAC=() LIN=()
tag=""
shopt -s nullglob
for d in "$DL_DIR"/*/; do
  an="$(basename "$d")"                     # already the public name, sans extension
  case "$NETWORK" in
    mainnet) [[ "$an" == *-testnet-v* ]] && continue ;;
    testnet) [[ "$an" == *-testnet-v* ]] || continue ;;
  esac
  [ -z "$tag" ] && tag="$(printf '%s' "$an" | grep -oE 'v[0-9.]+\[[0-9a-f]{7,}\]' | head -1 || true)"
  for f in "$d"*; do
    [ -f "$f" ] || continue
    dest="${an}.$(ext_of "$f")"
    cp "$f" "$OUT_DIR/$dest"
    h="$(sha256sum "$OUT_DIR/$dest" | awk '{print $1}')"
    case "$dest" in
      *installer*) label="INST: " ;;
      *.zip)       label="ZIP: " ;;
      *)           label="" ;;
    esac
    line="${label}${URL_BASE}/${dest}"$'\n'"sha256: ${h}"$'\n'
    case "$an" in
      zano-win-*)   WIN+=("$line") ;;
      zano-macos-*) MAC+=("$line") ;;
      zano-linux-*) LIN+=("$line") ;;
    esac
  done
done

files=( "$OUT_DIR"/* )
if [ ${#files[@]} -eq 0 ]; then
  echo "no ${NETWORK} artifacts found under $DL_DIR - nothing to publish"
  exit 0
fi
total_size="$(du -sh "$OUT_DIR" 2>/dev/null | cut -f1)"

# --- upload over SSH (retried; the workflow job is marked non-fatal) ------------
: "${SSH_TARGET:?ZANO_BUILDS_SSH_TARGET secret is required}"
: "${SSH_KEY:?ZANO_BUILDS_OPENSSH_KEY secret is required}"
# This repo is public, so Actions logs are public. Mask the (private) host so it
# does not leak even if ssh/scp prints it in an error message.
echo "::add-mask::${SSH_TARGET#*@}"
mkdir -p ~/.ssh && chmod 700 ~/.ssh
printf '%s\n' "$SSH_KEY" > ~/.ssh/id_ci && chmod 600 ~/.ssh/id_ci
if [ -n "${KNOWN_HOSTS:-}" ]; then
  printf '%s\n' "$KNOWN_HOSTS" > ~/.ssh/known_hosts && chmod 644 ~/.ssh/known_hosts
  strict="yes"
else
  strict="accept-new"   # no pinned host key provided; trust on first use
fi

upload_once() {
  scp -P "${SSH_PORT:-22}" -i ~/.ssh/id_ci \
      -o HostKeyAlgorithms=ssh-ed25519 \
      -o UserKnownHostsFile="$HOME/.ssh/known_hosts" \
      -o StrictHostKeyChecking="$strict" \
      -- "$OUT_DIR"/* "$SSH_TARGET:$REMOTE_DIR/"
}

echo "uploading ${#files[@]} file(s) (${total_size}) to the build server..."
attempts="${UPLOAD_RETRIES:-3}"
ok=0
upload_secs=0
for n in $(seq 1 "$attempts"); do
  a0=$(date +%s)
  if upload_once; then upload_secs=$(( $(date +%s) - a0 )); ok=1; break; fi
  if [ "$n" -lt "$attempts" ]; then
    echo "upload attempt $n/$attempts failed; retrying in $((n * 15))s..." >&2
    sleep "$((n * 15))"
  fi
done
rm -f ~/.ssh/id_ci
if [ "$ok" -ne 1 ]; then
  echo "::error::failed to upload builds after $attempts attempts"
  exit 1
fi
dur="$(printf '%d:%02d' $((upload_secs / 60)) $((upload_secs % 60)))"
up_msg="uploaded ${#files[@]} file(s) (${total_size}) to the build server in ${dur} (m:ss)"
echo "$up_msg"

# --- manifest for signing (stdout + job summary) -------------------------------
emit() {
  echo "Zano ${NETWORK} build ${tag}"
  date -u +%Y-%m-%d
  echo
  if [ ${#WIN[@]} -gt 0 ]; then echo "Windows:"; printf '%s\n' "${WIN[@]}"; fi
  if [ ${#MAC[@]} -gt 0 ]; then echo "macOS:";   printf '%s\n' "${MAC[@]}"; fi
  if [ ${#LIN[@]} -gt 0 ]; then echo "Linux:";   printf '%s\n' "${LIN[@]}"; fi
}
mani="$(emit)"
echo "-------- manifest --------"
echo "$mani"
if [ -n "${GITHUB_STEP_SUMMARY:-}" ]; then
  {
    echo "$up_msg"
    echo
    echo '```'
    echo "$mani"
    echo '```'
  } >> "$GITHUB_STEP_SUMMARY"
fi
