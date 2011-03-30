#!/bin/bash

APP='Tetzle'
BUNDLE="$APP.app"
VERSION='1.2.1'

# Create disk folder
echo -n 'Copying application bundle... '
rm -f "${APP}_$VERSION.dmg"
rm -Rf "$APP"
mkdir "$APP"
cp -pf COPYING "$APP/COPYING.txt"
cp -Rpf "$BUNDLE" "$APP/"
EXE_PATH="$APP/$BUNDLE/Contents/MacOS/"
cp -f 'tools/mac/jpegtran' $EXE_PATH
cp -f 'tools/mac/jhead' $EXE_PATH
echo 'Done'

# Copy translations
echo -n 'Copying translations... '
TRANSLATIONS="$APP/$BUNDLE/Contents/Resources/translations"
mkdir "$TRANSLATIONS"
cp -Rpf translations/*.qm "$TRANSLATIONS"
echo 'Done'

# Copy Qt translations
echo -n 'Copying Qt translations... '
for translation in $(ls translations | grep qm | cut -d'.' -f1)
do
	LPROJ="$APP/$BUNDLE/Contents/Resources/${translation}.lproj"
	mkdir "$LPROJ"
	sed "s/????/${translation}/" < locversion.plist > "${LPROJ}/locversion.plist"

	QT_TRANSLATION="/Developer/Applications/Qt/translations/qt_${translation}.qm"
	if [ -e "$QT_TRANSLATION" ]; then
		cp -f "$QT_TRANSLATION" "$TRANSLATIONS"
	fi
done
echo 'Done'

# Copy frameworks and plugins
echo -n 'Copying frameworks and plugins... '
macdeployqt "$APP/$BUNDLE"
# Remove extra frameworks and plugins copied by macdeployqt with 4.7.2
rm -Rf "$APP/$BUNDLE/Contents/Frameworks/QtDeclarative.framework"
rm -Rf "$APP/$BUNDLE/Contents/Frameworks/QtNetwork.framework"
rm -Rf "$APP/$BUNDLE/Contents/Frameworks/QtScript.framework"
rm -Rf "$APP/$BUNDLE/Contents/Frameworks/QtSql.framework"
rm -Rf "$APP/$BUNDLE/Contents/Frameworks/QtSvg.framework"
rm -Rf "$APP/$BUNDLE/Contents/Frameworks/QtXmlPatterns.framework"
rm -Rf "$APP/$BUNDLE/Contents/PlugIns/bearer"
rm -Rf "$APP/$BUNDLE/Contents/PlugIns/qmltooling"
echo 'Done'

# Create disk image
echo -n 'Creating disk image... '
hdiutil create -quiet -ov -srcfolder "$APP" -format UDBZ -volname "$APP" "${APP}_$VERSION.dmg"
hdiutil internet-enable -quiet -yes "${APP}_$VERSION.dmg"
echo 'Done'

# Clean up disk folder
echo -n 'Cleaning up... '
rm -Rf "$APP"
echo 'Done'
