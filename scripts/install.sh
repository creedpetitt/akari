#!/bin/bash
set -e

echo "🏮 Akari Framework Installer"
echo "---------------------------"

if command -v python3 &>/dev/null; then
    echo "Bundling single-header library..."
    python3 scripts/gen_single_header.py
else
    echo "Error: python3 is required to bundle the library."
    exit 1
fi

echo "Compiling Akari CLI..."
gcc -O2 tools/akari_cli.c -o akari

echo "Installing Akari CLI to /usr/local/bin/akari..."
sudo mv akari /usr/local/bin/akari

echo "---------------------------"
echo "Installation complete!"
echo "You can now run 'akari init <project_name>' from any directory."
