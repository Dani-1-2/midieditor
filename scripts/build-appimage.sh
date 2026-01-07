#!/bin/bash
set -e

# Download linuxdeploy and Qt plugin
wget -q https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
wget -q https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage
chmod +x linuxdeploy*.AppImage

# Copy icon with the name expected by the desktop file
cp packaging/unix/midieditor/logo48.png packaging/unix/midieditor/midieditor.png
mv MidiEditor midieditor

# Set up environment for linuxdeploy
export LINUXDEPLOY_OUTPUT_VERSION=$(git describe --tags --always)
export QMAKE=/usr/bin/qmake6
export QML_SOURCES_PATHS=.
export EXTRA_QT_PLUGINS="multimedia"

# Create AppImage
./linuxdeploy-x86_64.AppImage \
  --appdir AppDir \
  --executable midieditor \
  --desktop-file packaging/unix/midieditor/MidiEditor.desktop \
  --icon-file packaging/unix/midieditor/midieditor.png \
  --library /usr/lib/x86_64-linux-gnu/libfluidsynth.so.3 \
  --library /usr/lib/x86_64-linux-gnu/libasound.so.2 \
  --plugin qt \
  --output appimage

# Rename AppImage to a more user-friendly name
mv MidiEditor-*.AppImage MidiEditor-Linux-x86_64.AppImage
