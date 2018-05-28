#!/bin/sh


echo -n 'Preparing files...'
cd ..

rm -f tetzle.desktop.in
cp tetzle.desktop tetzle.desktop.in
sed -e '/^Name\[/ d' \
	-e '/^GenericName\[/ d' \
	-e '/^Comment\[/ d' \
	-e '/^Icon/ d' \
	-e '/^Keywords/ d' \
	-i tetzle.desktop.in

rm -f tetzle.appdata.xml.in
cp tetzle.appdata.xml tetzle.appdata.xml.in
sed -e '/p xml:lang/ d' \
	-e '/summary xml:lang/ d' \
	-e '/name xml:lang/ d' \
	-e '/<developer_name>/ d' \
	-i tetzle.appdata.xml.in

cd po
echo ' DONE'


echo -n 'Extracting messages...'
xgettext --from-code=UTF-8 --output=description.pot \
	--package-name='Tetzle' --copyright-holder='Graeme Gott' \
	../*.in
sed 's/CHARSET/UTF-8/' -i description.pot
echo ' DONE'


echo -n 'Cleaning up...'
cd ..

rm -f tetzle.desktop.in
rm -f tetzle.appdata.xml.in

echo ' DONE'
