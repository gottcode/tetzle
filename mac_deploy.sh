#!/bin/bash

APP='Tetzle'
BUNDLE="$APP.app"
VERSION='1.2.1'

macdeployqt $BUNDLE -dmg
mv "$APP.dmg" "${APP}_$VERSION.dmg"
