#!/bin/bash
set -e

# Colors
GREEN='\033[0;32m'
NC='\033[0m'

REPO="creedpetitt/akari"
VERSION="v0.1.0"

echo "    .    .   .    .    .--. --.--"
echo "   / \   |  /    / \   |   )  |  "
echo "  /___\  |-'    /___\  |--'   |  "
echo " /     \ |  \  /     \ |  \   |  "
echo "'       \`'   \`'       \`'   \`--'--"
echo ""
echo " Akari $VERSION - Zero-Allocation C Framework"
echo ""

echo "Checking system dependencies..."
for cmd in gcc python3 git; do
    if ! command -v $cmd &> /dev/null; then
        echo "Error: $cmd is not installed."
        exit 1
    fi
done

if [ -f "scripts/gen_single_header.py" ]; then
    echo "Local repository detected."
    REPO_ROOT=$(pwd)
else
    echo "Remote installation. Fetching source..."
    REPO_ROOT=$(mktemp -d)
    git clone --quiet --depth 1 https://github.com/${REPO}.git "$REPO_ROOT"
    trap 'rm -rf "$REPO_ROOT"' EXIT
fi

echo "Bundling single-header library..."
cd "$REPO_ROOT"
python3 scripts/gen_single_header.py > /dev/null

echo "Compiling Akari CLI..."
gcc -O2 tools/akari_cli.c -o akari_bin

echo "Installing to /usr/local/bin/akari..."
if [ -w /usr/local/bin ]; then
    mv akari_bin /usr/local/bin/akari
else
    sudo mv akari_bin /usr/local/bin/akari
fi

echo ""
echo -e "${GREEN}✓ Akari Installation Successful!${NC}"
echo "Type 'akari init <name>' to start building."
