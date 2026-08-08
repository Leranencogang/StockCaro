#!/usr/bin/env bash
# Stockaro - One-Shot Setup & Run Script (Linux)
# Usage: bash setup.sh

set -e  # exit on any error

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$PROJECT_DIR"

# Color helpers
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'
CYAN='\033[0;36m'; BOLD='\033[1m'; RESET='\033[0m'

info()    { echo -e "${CYAN}[INFO]${RESET}  $*"; }
success() { echo -e "${GREEN}[OK]${RESET}    $*"; }
warn()    { echo -e "${YELLOW}[WARN]${RESET}  $*"; }
error()   { echo -e "${RED}[ERROR]${RESET} $*"; exit 1; }

echo -e "  ${BOLD}Gomoku AI Engine - Setup & Launch${RESET}"

# STEP 1 - Check & install C++ compiler
info "Checking C++ compiler..."

if ! command -v g++ &>/dev/null; then
    warn "g++ not found. Attempting to install build-essential..."
    if command -v apt-get &>/dev/null; then
        sudo apt-get update -qq && sudo apt-get install -y -qq build-essential
    elif command -v dnf &>/dev/null; then
        sudo dnf install -y gcc-c++ make
    elif command -v pacman &>/dev/null; then
        sudo pacman -S --noconfirm base-devel
    else
        error "Cannot detect package manager. Please install g++ manually."
    fi
fi

GCC_VER=$(g++ --version | head -1)
success "Compiler: $GCC_VER"

# STEP 2 - Check & install Node.js
info "Checking Node.js..."

if ! command -v node &>/dev/null; then
    warn "Node.js not found. Attempting to install via NodeSource..."
    if command -v apt-get &>/dev/null; then
        curl -fsSL https://deb.nodesource.com/setup_lts.x | sudo -E bash -
        sudo apt-get install -y nodejs
    elif command -v dnf &>/dev/null; then
        sudo dnf install -y nodejs
    elif command -v pacman &>/dev/null; then
        sudo pacman -S --noconfirm nodejs npm
    else
        error "Cannot detect package manager. Please install Node.js manually (https://nodejs.org)."
    fi
fi

NODE_VER=$(node --version)
NPM_VER=$(npm --version)
success "Node.js $NODE_VER  |  npm $NPM_VER"

# STEP 3 - Install Node.js dependencies (express)
info "Installing Node.js packages..."
npm install --silent
success "npm packages installed"

# STEP 4 - Compile C++ engine
info "Compiling Stockaro C++ engine..."
make clean 2>/dev/null || true
make -j"$(nproc)"
success "Engine compiled: ./Stockaro"

# STEP 5 - Run quick benchmark test
echo ""
info "Running engine self-test (benchmark)..."
echo "----------------------------------------"
./Stockaro --bench
echo "----------------------------------------"
success "Engine self-test passed!"

# STEP 6 - Launch web server
echo ""
echo -e "${GREEN}${BOLD}Setup complete!${RESET}"
echo -e "  Opening web server at ${CYAN}http://localhost:3000${RESET}"
echo -e "  Press ${BOLD}Ctrl+C${RESET} to stop."
echo ""
node server.js
