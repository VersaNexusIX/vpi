#!/usr/bin/env sh
set -e
VPI_VERSION="1.0.0"
VPI_REPO="https://versas.my.id/vpi"
RED='\033[1;31m'
GREEN='\033[1;32m'
YELLOW='\033[1;33m'
CYAN='\033[1;36m'
RESET='\033[0m'
info()    { printf "${CYAN}[INFO]${RESET} %s\n" "$1"; }
success() { printf "${GREEN}[OK]${RESET} %s\n" "$1"; }
warn()    { printf "${YELLOW}[WARN]${RESET} %s\n" "$1"; }
error()   { printf "${RED}[ERROR]${RESET} %s\n" "$1"; exit 1; }
detect_os() {
    OS="$(uname -s)"
    ARCH="$(uname -m)"
    case "$OS" in
        Linux)  PLATFORM="linux" ;;
        Darwin) PLATFORM="macos" ;;
        MINGW*|MSYS*|CYGWIN*) PLATFORM="windows" ;;
        *) error "Unsupported OS: $OS" ;;
    esac
    case "$ARCH" in
        x86_64|amd64) ARCH="amd64" ;;
        aarch64|arm64) ARCH="arm64" ;;
        armv7l) ARCH="armv7" ;;
        *) error "Unsupported architecture: $ARCH" ;;
    esac
}
check_deps() {
    for cmd in curl tar; do
        command -v "$cmd" >/dev/null 2>&1 || error "Required dependency not found: $cmd"
    done
}
install_vpi() {
    detect_os
    check_deps
    printf "\n"
    printf " __ ___ __ _\n"
    printf " \\ V / '_ \\| |\n"
    printf "  \\_/| .__/|_|\n"
    printf "     |_|\n\n"
    info "Installing VPI v${VPI_VERSION} for ${PLATFORM}/${ARCH}"
    info "Building from source..."
    TMP_DIR="$(mktemp -d)"
    trap 'rm -rf "$TMP_DIR"' EXIT
    SRC_URL="${VPI_REPO}/releases/v${VPI_VERSION}/vpi-${PLATFORM}-${ARCH}.tar.gz"
    info "Downloading: $SRC_URL"
    if ! curl -fsSL "$SRC_URL" -o "$TMP_DIR/vpi.tar.gz"; then
        warn "Binary release not found. Building from source..."
        build_from_source "$TMP_DIR"
        return
    fi
    tar -xzf "$TMP_DIR/vpi.tar.gz" -C "$TMP_DIR"
    INSTALL_DIR="/usr/local/bin"
    if [ ! -w "$INSTALL_DIR" ]; then
        INSTALL_DIR="$HOME/.local/bin"
        mkdir -p "$INSTALL_DIR"
    fi
    info "Installing to $INSTALL_DIR/vpi"
    cp "$TMP_DIR/vpi" "$INSTALL_DIR/vpi"
    chmod +x "$INSTALL_DIR/vpi"
    success "VPI installed to $INSTALL_DIR/vpi"
    if ! echo "$PATH" | grep -q "$INSTALL_DIR"; then
        warn "Add to PATH: export PATH=\"$INSTALL_DIR:\$PATH\""
    fi
    printf "\n"
    printf "${GREEN}Installation complete!${RESET}\n"
    printf "Run: ${CYAN}vpi help${RESET}\n\n"
}
build_from_source() {
    TMP="$1"
    for cmd in gcc make; do
        command -v "$cmd" >/dev/null 2>&1 || error "Build tool not found: $cmd. Install build-essential."
    done
    command -v curl-config >/dev/null 2>&1 || error "libcurl development headers not found. Install libcurl-dev."
    info "Downloading source code..."
    SRC_URL="${VPI_REPO}/releases/v${VPI_VERSION}/vpi-${VPI_VERSION}-src.tar.gz"
    curl -fsSL "$SRC_URL" -o "$TMP/vpi-src.tar.gz" || error "Failed to download source."
    tar -xzf "$TMP/vpi-src.tar.gz" -C "$TMP"
    cd "$TMP/vpi-${VPI_VERSION}"
    info "Compiling..."
    make -j"$(nproc 2>/dev/null || echo 2)"
    INSTALL_DIR="/usr/local/bin"
    if [ ! -w "$INSTALL_DIR" ]; then INSTALL_DIR="$HOME/.local/bin"; mkdir -p "$INSTALL_DIR"; fi
    cp vpi "$INSTALL_DIR/vpi"
    chmod +x "$INSTALL_DIR/vpi"
    success "Built and installed to $INSTALL_DIR/vpi"
}
install_vpi
