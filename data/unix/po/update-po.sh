#!/bin/sh


echo -n 'Preparing files...'
cd ..

rm -f tetzle.desktop.in
cp tetzle.desktop tetzle.desktop.in
sed -e '/^Name\[/ d' \
	-e '/^GenericName\[/ d' \
	-e '/^Comment\[/ d' \
	-e 's/^Name/_Name/' \
	-e 's/^GenericName/_GenericName/' \
	-e 's/^Comment/_Comment/' \
	-i tetzle.desktop.in

rm -f tetzle.appdata.xml.in
cp tetzle.appdata.xml tetzle.appdata.xml.in
sed -e '/p xml:lang/ d' \
	-e '/summary xml:lang/ d' \
	-e '/name xml:lang/ d' \
	-e 's/<p>/<_p>/' \
	-e 's/<\/p>/<\/_p>/' \
	-e 's/<summary>/<_summary>/' \
	-e 's/<\/summary>/<\/_summary>/' \
	-e 's/<name>/<_name>/' \
	-e 's/<\/name>/<\/_name>/' \
	-i tetzle.appdata.xml.in

cd po
echo ' DONE'


echo -n 'Updating translations...'
for POFILE in *.po;
do
	echo -n " $POFILE"
	msgmerge --quiet --update --backup=none $POFILE description.pot
done
echo ' DONE'


echo -n 'Merging translations...'
cd ..

intltool-merge --quiet --desktop-style po tetzle.desktop.in tetzle.desktop
rm -f tetzle.desktop.in

intltool-merge --quiet --xml-style po tetzle.appdata.xml.in tetzle.appdata.xml
echo >> tetzle.appdata.xml
rm -f tetzle.appdata.xml.in

echo ' DONE'
