#!/usr/bin/env bash
# Produce an x86_64 AppImage with Qt runtime libraries (Ubuntu 24.04 baseline).
set -euo pipefail
install_root=${1:?usage: package_linux.sh installation-directory}
root=$(pwd)
appdir="$root/AppDir"
tools="$root/.cache/appimage-tools"
mkdir -p "$tools"

fetch_tool() {
    local repo=$1 asset=$2 output=$3 sha=$4
    gh api --allow-escape-sequences -H 'Accept: application/octet-stream' \
        "repos/$repo/releases/assets/$asset" > "$output"
    printf '%s  %s\n' "$sha" "$output" | sha256sum -c -
    chmod +x "$output"
}
# Pin actual asset IDs and SHA-256 digests rather than executing an unchecked
# moving "continuous" download. A deleted asset requires an explicit update.
fetch_tool linuxdeploy/linuxdeploy 699514588 "$tools/linuxdeploy" \
    df908b19f1ed1cdafa79508d415aa12d6c56ce4d56d4149aa8c09286718706e5
fetch_tool linuxdeploy/linuxdeploy-plugin-qt 525032210 "$tools/linuxdeploy-plugin-qt" \
    cfc1055b2b9dbc08412b579f20990b7b41a17b61beaa5847dc9477c96c9e9617
export PATH="$tools:$PATH"
export APPIMAGE_EXTRACT_AND_RUN=1
export QMAKE="${QT_ROOT_DIR:?Qt SDK path is required}/bin/qmake"
# The SDK's optional TIFF/MNG codecs depend on old system libraries that
# Ubuntu 24.04 no longer provides. Curate only the packaging plugin tree;
# do not mutate the cached SDK or disguise a libtiff ABI mismatch with a symlink.
export ELYSIUM_REAL_QMAKE="$QMAKE"
export ELYSIUM_QT_PLUGINS="$tools/qt-plugins"
mkdir -p "$ELYSIUM_QT_PLUGINS"
cp -a "$("$QMAKE" -query QT_INSTALL_PLUGINS)/." "$ELYSIUM_QT_PLUGINS/"
rm -f "$ELYSIUM_QT_PLUGINS/imageformats/libqtiff.so" "$ELYSIUM_QT_PLUGINS/imageformats/libqmng.so"
cat > "$tools/qmake-for-packaging" <<'QMAKE_WRAPPER'
#!/usr/bin/env bash
set -euo pipefail
if [[ "$#" == 1 && "$1" == -query ]]; then
    "$ELYSIUM_REAL_QMAKE" -query | sed "s|^QT_INSTALL_PLUGINS:.*$|QT_INSTALL_PLUGINS:$ELYSIUM_QT_PLUGINS|"
elif [[ "$#" == 2 && "$1" == -query && "$2" == QT_INSTALL_PLUGINS ]]; then
    printf '%s\n' "$ELYSIUM_QT_PLUGINS"
else
    exec "$ELYSIUM_REAL_QMAKE" "$@"
fi
QMAKE_WRAPPER
chmod +x "$tools/qmake-for-packaging"
export QMAKE="$tools/qmake-for-packaging"
export EXTRA_PLATFORM_PLUGINS="libqoffscreen.so"
export EXTRA_QT_MODULES="svg"
export OUTPUT=ElysiumLauncher-Linux-x86_64.AppImage
export ARCH=x86_64

# CMake installs JARs and desktop metadata too; do not package just the ELF file.
mkdir -p "$appdir/usr"
cp -a "$install_root"/. "$appdir/usr/"
mkdir -p "$appdir/usr/share/doc/ElysiumLauncher"
cp LICENSE COPYING.md "$appdir/usr/share/doc/ElysiumLauncher/"
printf 'Source: https://github.com/artemii14/ubiquitous-happiness\nCommit: %s\n' \
    "$(git rev-parse HEAD)" > "$appdir/usr/share/doc/ElysiumLauncher/BUILD.txt"
if test -d "$QT_ROOT_DIR/licenses"; then
    cp -a "$QT_ROOT_DIR/licenses" "$appdir/usr/share/doc/ElysiumLauncher/Qt-licenses"
fi
"$tools/linuxdeploy" --appdir "$appdir" \
    --executable "$appdir/usr/bin/elysiumlauncher" \
    --desktop-file "$appdir/usr/share/applications/io.github.artemii14.ElysiumLauncher.desktop" \
    --icon-file program_info/io.github.artemii14.ElysiumLauncher_256.png \
    --plugin qt --output appimage

test -s "$OUTPUT"
chmod +x "$OUTPUT"
QT_QPA_PLATFORM=offscreen timeout 60s "./$OUTPUT" --version
sha256sum "$OUTPUT" > "$OUTPUT.sha256"
