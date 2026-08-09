#!/usr/bin/env bash

set -e

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$PROJECT_DIR"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
BOLD='\033[1m'
RESET='\033[0m'

info() { echo -e "${CYAN}[INFO]${RESET}  $*"; }
success() { echo -e "${GREEN}[OK]${RESET}    $*"; }
warn() { echo -e "${YELLOW}[WARN]${RESET}  $*"; }
error() { echo -e "${RED}[ERROR]${RESET} $*"; exit 1; }

echo -e "  ${BOLD}StockCaro AI Engine - macOS Setup & Launch${RESET}"
echo ""

info "Checking operating system..."

if [[ "$(uname -s)" != "Darwin" ]]; then
    error "This setup script is for macOS only."
fi

MACOS_VERSION="$(sw_vers -productVersion)"
success "macOS $MACOS_VERSION"

info "Checking Xcode Command Line Tools..."

if ! xcode-select -p &>/dev/null; then
    warn "Xcode Command Line Tools not found."
    xcode-select --install
    echo ""
    echo "Please finish the installation and run this script again."
    exit 0
fi

success "Xcode Command Line Tools installed"

info "Checking Homebrew..."

if ! command -v brew &>/dev/null; then
    error "Homebrew not found. Install Homebrew first: https://brew.sh"
fi

success "$(brew --version | head -1)"

info "Checking C++ compiler..."

if command -v clang++ &>/dev/null; then
    CXX="$(command -v clang++)"
elif command -v g++ &>/dev/null; then
    CXX="$(command -v g++)"
else
    error "C++ compiler not found."
fi

success "Compiler: $("$CXX" --version | head -1)"

info "Checking Node.js..."

if ! command -v node &>/dev/null; then
    warn "Node.js not found. Installing via Homebrew..."
    brew install node
fi

NODE_VER="$(node --version)"
NPM_VER="$(npm --version)"

success "Node.js $NODE_VER | npm $NPM_VER"

info "Installing Node.js packages..."

if [[ ! -f package.json ]]; then
    error "package.json not found."
fi

npm install --silent

success "npm packages installed"

info "Compiling StockCaro C++ engine..."

if [[ ! -f Makefile && ! -f makefile ]]; then
    error "Makefile not found."
fi

make clean 2>/dev/null || true

CPU_CORES="$(sysctl -n hw.logicalcpu)"

info "Using $CPU_CORES CPU threads..."

make -j"$CPU_CORES"

if [[ ! -f ./StockCaro ]]; then
    error "Compilation finished but ./StockCaro was not found."
fi

success "Engine compiled: ./StockCaro"

echo ""
info "Running engine self-test..."

echo "----------------------------------------"

./StockCaro --bench

echo "----------------------------------------"

success "Engine self-test passed!"

echo ""
echo -e "${GREEN}${BOLD}Setup complete!${RESET}"
echo ""
echo -e "  Web server: ${CYAN}http://localhost:3000${RESET}"
echo -e "  Press ${BOLD}Ctrl+C${RESET} to stop."
echo ""

node server.js