#!/usr/bin/env bash
# MOON MIC — macOS build: VST3 + AU (universal arm64 + x86_64), tests, AU validation.
#
# Usage (from anywhere):   bash mac/build_mac.sh [--install]
#   --install   also copy the plugins to ~/Library/Audio/Plug-Ins/{VST3,Components}
#
# Needs: Xcode or Command Line Tools (xcode-select --install), CMake >= 3.22 (brew install cmake), git.
# JUCE 9.0.4 is cloned to ~/JUCE if it is not there (override with JUCE_DIR=/path bash mac/build_mac.sh).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
JUCE_DIR="${JUCE_DIR:-$HOME/JUCE}"
BUILD="$ROOT/plugin/build-mac"
INSTALL=0
[[ "${1:-}" == "--install" ]] && INSTALL=1

echo "== MOON MIC macOS build"
echo "   project: $ROOT"
echo "   JUCE:    $JUCE_DIR"

command -v cmake >/dev/null || { echo "CMake not found: brew install cmake"; exit 1; }
xcode-select -p >/dev/null 2>&1 || { echo "Xcode tools not found: xcode-select --install"; exit 1; }

if [[ ! -f "$JUCE_DIR/CMakeLists.txt" ]]; then
    echo "== cloning JUCE 9.0.4 to $JUCE_DIR"
    git clone --branch 9.0.4 --depth 1 https://github.com/juce-framework/JUCE.git "$JUCE_DIR"
fi

GEN="Unix Makefiles"
command -v ninja >/dev/null && GEN="Ninja"
xcodebuild -version >/dev/null 2>&1 && GEN="Xcode"
echo "== generator: $GEN"

cmake -S "$ROOT/plugin" -B "$BUILD" -G "$GEN" \
      -DJUCE_DIR="$JUCE_DIR" \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
      -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0

cmake --build "$BUILD" --config Release --parallel \
      --target MoonMic_VST3 MoonMic_AU MoonMicTests MoonMicProcessorTests

ART="$BUILD/MoonMic_artefacts/Release"
VST3="$ART/VST3/MOON MIC.vst3"
AU="$ART/AU/MOON MIC.component"
[[ -d "$VST3" ]] || { echo "VST3 not found at $VST3"; exit 1; }
[[ -d "$AU" ]]   || { echo "AU not found at $AU"; exit 1; }

echo "== tests"
"$BUILD/MoonMicTests_artefacts/Release/MoonMicTests"
"$BUILD/MoonMicProcessorTests_artefacts/Release/MoonMicProcessorTests"

if [[ $INSTALL -eq 1 ]]; then
    echo "== installing to ~/Library/Audio/Plug-Ins"
    mkdir -p "$HOME/Library/Audio/Plug-Ins/VST3" "$HOME/Library/Audio/Plug-Ins/Components"
    rm -rf "$HOME/Library/Audio/Plug-Ins/VST3/MOON MIC.vst3" "$HOME/Library/Audio/Plug-Ins/Components/MOON MIC.component"
    cp -R "$VST3" "$HOME/Library/Audio/Plug-Ins/VST3/"
    cp -R "$AU" "$HOME/Library/Audio/Plug-Ins/Components/"
    killall -9 AudioComponentRegistrar >/dev/null 2>&1 || true   # make macOS rescan AUs
    echo "== AU validation"
    auval -v aufx MnMc MnAE
fi

echo
echo "== done"
echo "   VST3: $VST3"
echo "   AU:   $AU"
[[ $INSTALL -eq 0 ]] && echo "   (run with --install to copy them to ~/Library/Audio/Plug-Ins and run auval)"
