#!/usr/bin/env sh
set -e
GREEN='\033[1;32m'; CYAN='\033[1;36m'; RED='\033[1;31m'; RESET='\033[0m'
info() { printf "${CYAN}[INFO]${RESET} %s\n" "$1"; }
ok()   { printf "${GREEN}[OK]${RESET} %s\n" "$1"; }
fail() { printf "${RED}[ERROR]${RESET} %s\n" "$1"; exit 1; }
printf "\n${CYAN} VPI Build Bootstrap${RESET}\n\n"
for cmd in gcc make; do
    command -v "$cmd" >/dev/null 2>&1 || fail "Missing tool: $cmd"
done
pkg-config --exists libcurl 2>/dev/null || \
    curl-config --version >/dev/null 2>&1 || \
    fail "libcurl-dev not found. Install: apt install libcurl4-openssl-dev"
pkg-config --exists zlib 2>/dev/null || \
    fail "zlib-dev not found. Install: apt install zlib1g-dev"
info "Building VPI..."
make -j"$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2)"
ok "Build complete: ./vpi"
printf "\nRun:     ${CYAN}./vpi help${RESET}\n"
printf "Install: ${CYAN}sudo make install${RESET}\n\n"
