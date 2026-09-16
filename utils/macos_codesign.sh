#!/usr/bin/env bash
#
# macOS code signing and notarization helpers for CI.
#
# A Developer ID signature on its own does not get past Gatekeeper: since macOS
# 10.15 anything a user downloads must ALSO carry a notarization ticket. So the
# pipeline for a shippable artifact is always
#
#     sign (hardened runtime + secure timestamp) -> notarize -> staple -> verify
#
# Subcommands
#   keychain                    create a temp keychain, import the .p12, pick the identity
#   sign-binaries FILE...       sign standalone Mach-O executables
#   sign-app APP ENTITLEMENTS   sign a Qt .app bundle inside-out
#   sign-dmg DMG                sign a disk image
#   notarize FILE               submit a .dmg/.pkg/.zip to Apple, wait, staple when possible
#   notarize-app APP            zip the bundle, notarize it, staple the ticket into the bundle
#   verify-app APP              assert the bundle is signed, hardened, notarized and stapled
#   verify-dmg DMG              same for a disk image
#   cleanup                     remove the temp keychain and restore the search list
#
# Environment
#   keychain       MACOS_SIGN_CERT_P12, MACOS_SIGN_CERT_PASSWORD
#                  MACOS_SIGN_IDENTITY (optional: exact identity or SHA-1, for cert rotation)
#                  MACOS_TEAM_ID       (optional: asserted against the identity)
#   sign-*/verify  SIGN_IDENTITY, SIGN_KEYCHAIN  (exported by `keychain`)
#   notarize       MACOS_NOTARY_API_KEY_P8, MACOS_NOTARY_API_KEY_ID, MACOS_NOTARY_API_ISSUER_ID
#
# Written for the bash 3.2 that ships with macOS - no mapfile, no associative arrays.

set -euo pipefail

readonly SEARCH_LIST_BACKUP="${RUNNER_TEMP:-/tmp}/zano-keychain-search-list.txt"

die() { echo "::error::$*" >&2; exit 1; }
note() { echo "==> $*"; }

require_env() {
  local name
  for name in "$@"; do
    eval "local value=\${$name:-}"
    [ -n "$value" ] || die "$name is not set"
  done
}

require_signing_context() {
  require_env SIGN_IDENTITY SIGN_KEYCHAIN
  [ -f "$SIGN_KEYCHAIN" ] || die "signing keychain not found at $SIGN_KEYCHAIN (run 'keychain' first)"
}

# codesign with the settings notarization requires: hardened runtime and a
# secure timestamp. Callers add --entitlements where a binary needs them.
codesign_it() {
  codesign --force --options runtime --timestamp \
    --keychain "$SIGN_KEYCHAIN" --sign "$SIGN_IDENTITY" "$@"
}

# ---------------------------------------------------------------- keychain ---

cmd_keychain() {
  require_env MACOS_SIGN_CERT_P12 MACOS_SIGN_CERT_PASSWORD
  local keychain="${RUNNER_TEMP:?RUNNER_TEMP is not set}/zano-signing.keychain-db"
  local keychain_password p12
  keychain_password="$(uuidgen)"
  p12="${RUNNER_TEMP}/zano-signing.p12"

  security create-keychain -p "$keychain_password" "$keychain"
  # -lut 21600: auto-lock after 6h of idle, so a hung job cannot leave it open forever.
  security set-keychain-settings -lut 21600 "$keychain"
  security unlock-keychain -p "$keychain_password" "$keychain"

  printf '%s' "$MACOS_SIGN_CERT_P12" | base64 -d > "$p12"
  # -T grants these tools access without an interactive prompt. security and
  # productsign are listed as well as codesign so stapling/packaging keep working.
  security import "$p12" -k "$keychain" -P "$MACOS_SIGN_CERT_PASSWORD" \
    -T /usr/bin/codesign -T /usr/bin/security -T /usr/bin/productsign
  rm -f "$p12"

  # Without this, codesign blocks on a UI prompt that never comes on a runner.
  # 'codesign:' belongs in the partition list alongside the apple partitions.
  security set-key-partition-list \
    -S apple-tool:,apple:,codesign: -s -k "$keychain_password" "$keychain" > /dev/null

  # PREPEND to the search list rather than replacing it. Replacing it drops the
  # login and system keychains, and the Developer ID intermediate CA lives
  # there - without it the chain will not build and find-identity shows nothing.
  local existing_keychains=()
  local line trimmed
  security list-keychains -d user > "$SEARCH_LIST_BACKUP"
  while IFS= read -r line; do
    trimmed="$(printf '%s' "$line" | sed -e 's/^[[:space:]]*"\{0,1\}//' -e 's/"\{0,1\}[[:space:]]*$//')"
    [ -n "$trimmed" ] && existing_keychains+=("$trimmed")
  done < "$SEARCH_LIST_BACKUP"
  security list-keychains -d user -s "$keychain" ${existing_keychains[@]+"${existing_keychains[@]}"}

  # Pick the signing identity. 'first match wins' silently picks the wrong
  # certificate during a rotation when old and new overlap, so require an
  # unambiguous choice and let MACOS_SIGN_IDENTITY break a tie explicitly.
  local identity_list identity_count identity
  identity_list="$(security find-identity -v -p codesigning "$keychain" \
    | grep 'Developer ID Application' || true)"
  [ -n "$identity_list" ] \
    || die "no 'Developer ID Application' identity in the imported .p12 (is it a Developer ID Application cert, exported WITH its private key?)"

  if [ -n "${MACOS_SIGN_IDENTITY:-}" ]; then
    identity="$MACOS_SIGN_IDENTITY"
    printf '%s\n' "$identity_list" | grep -qF "$identity" \
      || die "MACOS_SIGN_IDENTITY ($identity) does not match any identity in the .p12:"$'\n'"$identity_list"
  else
    identity_count="$(printf '%s\n' "$identity_list" | grep -c 'Developer ID Application')"
    [ "$identity_count" -eq 1 ] \
      || die "the .p12 holds $identity_count Developer ID Application identities; set MACOS_SIGN_IDENTITY to choose:"$'\n'"$identity_list"
    identity="$(printf '%s\n' "$identity_list" | sed -n 's/.*"\(.*\)".*/\1/p' | head -1)"
  fi

  if [ -n "${MACOS_TEAM_ID:-}" ]; then
    case "$identity" in
      *"($MACOS_TEAM_ID)"*) : ;;
      *) die "identity '$identity' does not belong to team $MACOS_TEAM_ID" ;;
    esac
  fi

  # Apple's notary service rejects submissions whose chain cannot be built, and
  # a .p12 exported without the intermediate is the usual cause. Warn rather
  # than fail: the intermediate may legitimately come from the system keychain.
  security find-certificate -c 'Developer ID Certification Authority' -a "$keychain" > /dev/null 2>&1 \
    || echo "::warning title=Signing certificate::The .p12 does not carry the 'Developer ID Certification Authority' intermediate. If notarization fails to build the chain, re-export the .p12 with the full chain."

  note "signing identity: $identity"
  if [ -n "${GITHUB_ENV:-}" ]; then
    {
      echo "SIGN_IDENTITY=$identity"
      echo "SIGN_KEYCHAIN=$keychain"
    } >> "$GITHUB_ENV"
  fi
}

# ------------------------------------------------------------ sign binaries ---

cmd_sign_binaries() {
  require_signing_context
  [ "$#" -gt 0 ] || die "sign-binaries needs at least one file"
  local binary
  for binary in "$@"; do
    [ -f "$binary" ] || die "not a file: $binary"
    note "signing $binary"
    codesign_it "$binary"
    codesign --verify --strict --verbose=2 "$binary"
  done
}

# ----------------------------------------------------------------- sign app ---

# Sign a bundle inside-out. `codesign --deep` is the obvious-looking alternative
# and it is wrong twice over: Apple deprecates it for signing, and it stamps the
# OUTER bundle's entitlements onto every nested binary. For a Qt WebEngine app
# that is not cosmetic - QtWebEngineProcess ships its own entitlements, and with
# the hardened runtime plus the wrong ones it crashes on launch, taking the
# whole UI with it.
cmd_sign_app() {
  require_signing_context
  local app="${1:?sign-app needs the .app path}"
  local entitlements="${2:?sign-app needs an entitlements plist}"
  [ -d "$app" ] || die "not a bundle: $app"
  [ -f "$entitlements" ] || die "entitlements not found: $entitlements"

  # Extended attributes and .DS_Store files make codesign fail with
  # "resource fork, Finder information, or similar detritus not allowed".
  note "clearing extended attributes"
  xattr -cr "$app"
  find "$app" -name '.DS_Store' -type f -delete

  # codesign treats Contents/MacOS as a code directory. Anything in there that
  # is not a Mach-O binary becomes an unsigned "subcomponent" and fails
  # --strict verification (and notarization after it). Catch it up front, and
  # name the offender, instead of failing at the end with codesign's opaque
  # "code object is not signed at all / In subcomponent: ..." message.
  local stray=""
  local item
  for item in "$app"/Contents/MacOS/*; do
    [ -e "$item" ] || continue
    if [ -d "$item" ]; then
      stray="${stray}"$'\n'"  $(basename "$item")/ (directory; resources belong in Contents/Resources)"
    elif ! file -b "$item" | grep -q 'Mach-O'; then
      stray="${stray}"$'\n'"  $(basename "$item") (not a Mach-O executable)"
    fi
  done
  if [ -n "$stray" ]; then
    die "non-code entries in $app/Contents/MacOS will fail signature verification:${stray}"
  fi

  # 1. The Qt WebEngine helper, with the entitlements Qt ships for it.
  local helper helper_entitlements
  helper="$(find "$app/Contents" -type d -name 'QtWebEngineProcess.app' -print 2>/dev/null | head -1 || true)"
  if [ -n "$helper" ]; then
    helper_entitlements="$helper/Contents/Resources/QtWebEngineProcess.entitlements"
    [ -f "$helper_entitlements" ] \
      || die "found $helper but not its QtWebEngineProcess.entitlements - signing it with the app's entitlements would break the renderer"
    note "signing Qt WebEngine helper with its own entitlements"
    codesign_it --entitlements "$helper_entitlements" "$helper/Contents/MacOS/QtWebEngineProcess"
    codesign_it --entitlements "$helper_entitlements" "$helper"
  else
    echo "::warning title=Code signing::No QtWebEngineProcess.app found under $app/Contents - check that macdeployqt ran."
  fi

  # 2. Loose Mach-O libraries, deepest path first. Reverse lexicographic order
  #    puts a child path ahead of its parent, which is all "inside-out" needs.
  find "$app/Contents" \( -name '*.dylib' -o -name '*.so' \) -type f \
    | sort -r \
    | while IFS= read -r item; do
        codesign_it "$item"
      done

  # 3. Frameworks and any remaining nested bundles, again deepest first. The
  #    WebEngine helper is inside QtWebEngineCore.framework, so it had to be
  #    signed before this step - it was, in step 1.
  find "$app/Contents" -type d \( -name '*.framework' -o -name '*.bundle' -o -name '*.appex' \) \
    | sort -r \
    | while IFS= read -r item; do
        codesign_it "$item"
      done

  # 4. The auxiliary executables the build drops into Contents/MacOS. These are
  #    full programs, not plugins, and each needs its own signature.
  local main_executable
  main_executable="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleExecutable' "$app/Contents/Info.plist" 2>/dev/null || echo '')"
  for item in "$app"/Contents/MacOS/*; do
    [ -f "$item" ] || continue   # skips the html/ tree the build copies in here
    [ -x "$item" ] || continue
    if [ -n "$main_executable" ] && [ "$(basename "$item")" = "$main_executable" ]; then
      continue  # signed as part of the bundle in step 5
    fi
    note "signing auxiliary executable $(basename "$item")"
    codesign_it --entitlements "$entitlements" "$item"
  done

  # 5. The outer bundle last, so it seals everything signed above.
  note "signing $app"
  codesign_it --entitlements "$entitlements" "$app"

  # --deep on verify is not the same flag as --deep on sign: here it means
  # "also validate nested code", which is exactly what we want.
  codesign --verify --deep --strict --verbose=2 "$app"
}

# ----------------------------------------------------------------- sign dmg ---

cmd_sign_dmg() {
  require_signing_context
  local dmg="${1:?sign-dmg needs the .dmg path}"
  [ -f "$dmg" ] || die "not a file: $dmg"
  note "signing $dmg"
  # A disk image is not executable code, so the hardened runtime does not apply;
  # it still needs a signature to carry a stapled ticket.
  codesign --force --timestamp --keychain "$SIGN_KEYCHAIN" --sign "$SIGN_IDENTITY" "$dmg"
  codesign --verify --strict --verbose=2 "$dmg"
}

# ----------------------------------------------------------------- notarize ---

cmd_notarize() {
  require_env MACOS_NOTARY_API_KEY_P8 MACOS_NOTARY_API_KEY_ID MACOS_NOTARY_API_ISSUER_ID
  local target="${1:?notarize needs a file}"
  [ -e "$target" ] || die "not found: $target"

  # notarytool accepts only UDIF disk images, signed flat packages and zips.
  case "$target" in
    *.dmg|*.pkg|*.zip) : ;;
    *) die "notarytool cannot accept $target - submit a .dmg, .pkg or .zip" ;;
  esac

  local key="${RUNNER_TEMP:?RUNNER_TEMP is not set}/zano-notary-key.p8"
  printf '%s' "$MACOS_NOTARY_API_KEY_P8" | base64 -d > "$key"
  # The .p8 is a credential; make sure it goes away whatever happens next.
  trap 'rm -f "$key"' EXIT

  note "submitting $(basename "$target") to the notary service"
  local result_json="${RUNNER_TEMP}/zano-notary-result.json"
  local submission_id submission_status
  set +e
  xcrun notarytool submit "$target" \
    --key "$key" \
    --key-id "$MACOS_NOTARY_API_KEY_ID" \
    --issuer "$MACOS_NOTARY_API_ISSUER_ID" \
    --wait --timeout 45m --output-format json > "$result_json"
  set -e
  cat "$result_json"

  # Do not trust the exit code alone: a submission that finishes processing as
  # Invalid has historically still exited 0. Assert the status explicitly.
  submission_id="$(jq -r '.id // empty' "$result_json")"
  submission_status="$(jq -r '.status // empty' "$result_json")"

  if [ "$submission_status" != "Accepted" ]; then
    if [ -n "$submission_id" ]; then
      echo "--- notarization log for $submission_id ---"
      xcrun notarytool log "$submission_id" \
        --key "$key" --key-id "$MACOS_NOTARY_API_KEY_ID" --issuer "$MACOS_NOTARY_API_ISSUER_ID" || true
    fi
    die "notarization returned status '${submission_status:-unknown}' for $target"
  fi

  rm -f "$key"
  trap - EXIT

  # A ticket can only be attached to a container that has somewhere to put it.
  # A bare Mach-O has nowhere, so CLI binaries shipped loose rely on Gatekeeper's
  # online check instead. That still works; it just needs connectivity once.
  case "$target" in
    *.dmg|*.pkg)
      note "stapling $target"
      xcrun stapler staple "$target"
      xcrun stapler validate "$target"
      ;;
    *.zip)
      note "notarized; a .zip cannot be stapled (the ticket is served online instead)"
      ;;
  esac
}

# Notarize a .app and staple the ticket into the bundle itself. Stapling the
# app (not just the disk image it ships in) is what lets it launch offline once
# the user has dragged it out of the .dmg.
cmd_notarize_app() {
  local app="${1:?notarize-app needs the .app path}"
  [ -d "$app" ] || die "not a bundle: $app"
  local archive="${RUNNER_TEMP:?RUNNER_TEMP is not set}/$(basename "$app").zip"

  # ditto, not zip: it preserves the bundle's symlinks and the signature.
  rm -f "$archive"
  /usr/bin/ditto -c -k --keepParent "$app" "$archive"
  cmd_notarize "$archive"
  rm -f "$archive"

  note "stapling $app"
  xcrun stapler staple "$app"
  xcrun stapler validate "$app"
}

cmd_staple() {
  local target="${1:?staple needs a path}"
  note "stapling $target"
  xcrun stapler staple "$target"
  xcrun stapler validate "$target"
}

# ------------------------------------------------------------------- verify ---

cmd_verify_app() {
  local app="${1:?verify-app needs the .app path}"
  echo "--- codesign ---"
  codesign --verify --deep --strict --verbose=4 "$app"
  codesign --display --verbose=4 "$app" 2>&1 | sed -n '1,12p'
  echo "--- hardened runtime ---"
  codesign --display --verbose=2 "$app" 2>&1 | grep -q 'flags=.*runtime' \
    || die "$app is not signed with the hardened runtime"
  echo "--- notarization ticket ---"
  xcrun stapler validate "$app" || die "$app has no stapled notarization ticket"
  echo "--- Gatekeeper ---"
  spctl --assess --verbose=4 --type exec "$app"
}

cmd_verify_dmg() {
  local dmg="${1:?verify-dmg needs the .dmg path}"
  echo "--- codesign ---"
  codesign --verify --strict --verbose=2 "$dmg"
  echo "--- notarization ticket ---"
  xcrun stapler validate "$dmg" || die "$dmg has no stapled notarization ticket"
  echo "--- Gatekeeper ---"
  spctl --assess --verbose=4 --type open --context context:primary-signature "$dmg"
}

# ------------------------------------------------------------------ cleanup ---

cmd_cleanup() {
  local keychain="${SIGN_KEYCHAIN:-${RUNNER_TEMP:-/tmp}/zano-signing.keychain-db}"
  if [ -f "$SEARCH_LIST_BACKUP" ]; then
    local restored=()
    local line trimmed
    while IFS= read -r line; do
      trimmed="$(printf '%s' "$line" | sed -e 's/^[[:space:]]*"\{0,1\}//' -e 's/"\{0,1\}[[:space:]]*$//')"
      [ -n "$trimmed" ] && restored+=("$trimmed")
    done < "$SEARCH_LIST_BACKUP"
    [ "${#restored[@]}" -gt 0 ] && security list-keychains -d user -s "${restored[@]}" || true
    rm -f "$SEARCH_LIST_BACKUP"
  fi
  if [ -f "$keychain" ]; then
    security delete-keychain "$keychain" || true
  fi
  rm -f "${RUNNER_TEMP:-/tmp}/zano-signing.p12" "${RUNNER_TEMP:-/tmp}/zano-notary-key.p8"
  note "signing material removed"
}

# --------------------------------------------------------------------- main ---

command="${1:-}"
[ "$#" -gt 0 ] && shift || true
case "$command" in
  keychain)      cmd_keychain "$@" ;;
  sign-binaries) cmd_sign_binaries "$@" ;;
  sign-app)      cmd_sign_app "$@" ;;
  sign-dmg)      cmd_sign_dmg "$@" ;;
  notarize)      cmd_notarize "$@" ;;
  notarize-app)  cmd_notarize_app "$@" ;;
  staple)        cmd_staple "$@" ;;
  verify-app)    cmd_verify_app "$@" ;;
  verify-dmg)    cmd_verify_dmg "$@" ;;
  cleanup)       cmd_cleanup "$@" ;;
  *)
    sed -n '2,28p' "$0"
    die "unknown subcommand: ${command:-<none>}"
    ;;
esac
