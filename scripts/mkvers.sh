#!/usr/bin/env sh
set -e
RED='\033[1;31m'; GREEN='\033[1;32m'; CYAN='\033[1;36m'; RESET='\033[0m'
usage() {
    printf "Usage: mkvers <package-dir> [output-name]\n"
    printf "  Creates a .vers package from a directory.\n"
    printf "  The directory MUST contain a vers.json file.\n\n"
    printf "  vers.json required fields:\n"
    printf "    nama, versi, developer, lisensi\n\n"
    printf "  Example:\n"
    printf "    ./mkvers ./mypackage mypackage\n"
    exit 1
}
[ "$#" -lt 1 ] && usage
PKG_DIR="$1"
PKG_NAME="${2:-$(basename "$PKG_DIR")}"
[ ! -d "$PKG_DIR" ] && printf "${RED}Error:${RESET} Directory not found: %s\n" "$PKG_DIR" && exit 1
VERS_JSON="$PKG_DIR/vers.json"
[ ! -f "$VERS_JSON" ] && printf "${RED}Error:${RESET} vers.json not found in %s\n" "$PKG_DIR" && exit 1
printf "${CYAN}Validating vers.json...${RESET}\n"
for field in nama versi developer lisensi; do
    if ! grep -q "\"$field\"" "$VERS_JSON"; then
        printf "${RED}Error:${RESET} Missing required field: '%s' in vers.json\n" "$field"
        exit 1
    fi
    printf "  ${GREEN}✓${RESET} %s\n" "$field"
done
OUTPUT="${PKG_NAME}.vers"
printf "\n${CYAN}Creating package:${RESET} %s\n" "$OUTPUT"
ORIG_DIR="$(pwd)"
cd "$PKG_DIR"
zip -r "$ORIG_DIR/$OUTPUT" . -x "*.DS_Store" -x "__MACOSX/*"
cd "$ORIG_DIR"
SIZE=$(du -sh "$OUTPUT" | cut -f1)
printf "${GREEN}[OK]${RESET} Created: %s (%s)\n" "$OUTPUT" "$SIZE"
printf "\nUpload to: https://versas.my.id/package/%s.vers\n" "$PKG_NAME"
