#!/usr/bin/env bash
# Packages a built .gbc into a versioned, checksummed release artifact.
# Called by `make release`; expects VERSION, PROJECT, BUILDDIR, RELEASEDIR
# to be exported by the Makefile. BUILD_NUMBER (the GitHub Actions run
# number in CI, "local" otherwise) drives the filename so it increments
# by one on every CI build and always matches the version shown on the
# title screen (see BUILD_NUMBER in src/main.c).
set -euo pipefail

VERSION="${VERSION:?VERSION not set}"
PROJECT="${PROJECT:?PROJECT not set}"
BUILD_NUMBER="${BUILD_NUMBER:-local}"
BUILDDIR="${BUILDDIR:-build}"
RELEASEDIR="${RELEASEDIR:-release}"

ROM="$BUILDDIR/$PROJECT.gbc"
if [ ! -f "$ROM" ]; then
    echo "error: $ROM not found — run 'make all' first" >&2
    exit 1
fi

mkdir -p "$RELEASEDIR"

OUT_NAME="${PROJECT}-${BUILD_NUMBER}"
OUT_ROM="$RELEASEDIR/${OUT_NAME}.gbc"

cp "$ROM" "$OUT_ROM"

# sha256 checksum alongside the ROM, so players/distributors can verify
# the file wasn't corrupted or tampered with after release.
if command -v sha256sum >/dev/null 2>&1; then
    sha256sum "$OUT_ROM" > "${OUT_ROM}.sha256"
elif command -v shasum >/dev/null 2>&1; then
    shasum -a 256 "$OUT_ROM" > "${OUT_ROM}.sha256"
fi

# Bundle ROM + checksum + symbol file (if present, for post-release debugging)
ZIP_PATH="$RELEASEDIR/${OUT_NAME}.zip"
FILES_TO_ZIP=("$OUT_ROM" "${OUT_ROM}.sha256")
[ -f "$BUILDDIR/$PROJECT.sym" ] && FILES_TO_ZIP+=("$BUILDDIR/$PROJECT.sym")

if command -v zip >/dev/null 2>&1; then
    zip -j "$ZIP_PATH" "${FILES_TO_ZIP[@]}" >/dev/null
    echo "Release packaged: $ZIP_PATH"
else
    echo "Release ROM ready: $OUT_ROM (zip not found on PATH, skipped archiving)"
fi
