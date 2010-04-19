#!/bin/bash

APP='Tetzle'
BUNDLE="$APP.app"
VERSION='1.2.1'

EXE_PATH="$BUNDLE/Contents/MacOS/"
cp -f 'tools/mac/jpegtran' $EXE_PATH
cp -f 'tools/mac/jhead' $EXE_PATH

macdeployqt $BUNDLE -dmg
mv "$APP.dmg" "${APP}_$VERSION.dmg"
