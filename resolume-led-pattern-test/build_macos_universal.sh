#!/bin/bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
SDK_DIR="${1:-$ROOT_DIR/../ffgl}"
OUTPUT_DIR="${2:-$ROOT_DIR/dist-macos}"
BUNDLE_NAME="PackItLEDPattern.bundle"
EXECUTABLE_NAME="PackItLEDPattern"
BUNDLE_DIR="$OUTPUT_DIR/$BUNDLE_NAME"
MACOS_DIR="$BUNDLE_DIR/Contents/MacOS"

if [[ ! -f "$SDK_DIR/source/lib/FFGLSDK.cpp" ]]; then
  echo "FFGL SDK not found: $SDK_DIR" >&2
  echo "Pass the path to the official resolume/ffgl checkout as the first argument." >&2
  exit 1
fi

rm -rf "$OUTPUT_DIR"
mkdir -p "$MACOS_DIR"

cat > "$BUNDLE_DIR/Contents/Info.plist" <<'PLIST'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>CFBundleDevelopmentRegion</key>
  <string>English</string>
  <key>CFBundleExecutable</key>
  <string>PackItLEDPattern</string>
  <key>CFBundleIdentifier</key>
  <string>it.pack.resolume.ledpattern</string>
  <key>CFBundleInfoDictionaryVersion</key>
  <string>6.0</string>
  <key>CFBundleName</key>
  <string>PackItLEDPattern</string>
  <key>CFBundlePackageType</key>
  <string>BNDL</string>
  <key>CFBundleShortVersionString</key>
  <string>0.2</string>
  <key>CFBundleVersion</key>
  <string>2</string>
  <key>LSMinimumSystemVersion</key>
  <string>11.0</string>
  <key>NSHighResolutionCapable</key>
  <true/>
</dict>
</plist>
PLIST

SDKROOT="$(xcrun --sdk macosx --show-sdk-path)"

clang++ \
  -std=c++17 \
  -O3 \
  -DNDEBUG \
  -fPIC \
  -bundle \
  -arch arm64 \
  -arch x86_64 \
  -mmacosx-version-min=11.0 \
  -isysroot "$SDKROOT" \
  -I"$SDK_DIR/source/lib" \
  "$SDK_DIR/source/lib/FFGLSDK.cpp" \
  "$ROOT_DIR/plugin/PackItLEDPattern.cpp" \
  -framework OpenGL \
  -o "$MACOS_DIR/$EXECUTABLE_NAME"

chmod +x "$MACOS_DIR/$EXECUTABLE_NAME"
plutil -lint "$BUNDLE_DIR/Contents/Info.plist"

ARCHS="$(lipo -archs "$MACOS_DIR/$EXECUTABLE_NAME")"
echo "Architectures: $ARCHS"
[[ "$ARCHS" == *"arm64"* ]] || { echo "arm64 architecture is missing" >&2; exit 1; }
[[ "$ARCHS" == *"x86_64"* ]] || { echo "x86_64 architecture is missing" >&2; exit 1; }

nm -gU "$MACOS_DIR/$EXECUTABLE_NAME" | grep -E ' _plugMain$' >/dev/null || {
  echo "Required FFGL entry point _plugMain was not exported" >&2
  exit 1
}

codesign --force --sign - --timestamp=none "$BUNDLE_DIR"
codesign --verify --deep --strict --verbose=2 "$BUNDLE_DIR"

cp "$ROOT_DIR/INSTALL_MAC_RU.txt" "$OUTPUT_DIR/INSTALL_MAC_RU.txt"

PACKAGE="$OUTPUT_DIR/PackItLEDPattern-v0.2a-mac-universal.zip"
(
  cd "$OUTPUT_DIR"
  ditto -c -k --sequesterRsrc --keepParent "$BUNDLE_NAME" "$(basename "$PACKAGE")"
)

TMP_UNZIP="$OUTPUT_DIR/package-temp"
mkdir -p "$TMP_UNZIP"
ditto -x -k "$PACKAGE" "$TMP_UNZIP"
cp "$ROOT_DIR/INSTALL_MAC_RU.txt" "$TMP_UNZIP/INSTALL_MAC_RU.txt"
rm -f "$PACKAGE"
(
  cd "$TMP_UNZIP"
  ditto -c -k --sequesterRsrc . "$PACKAGE"
)
rm -rf "$TMP_UNZIP"

echo "Built: $PACKAGE"
