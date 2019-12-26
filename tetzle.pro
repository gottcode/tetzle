lessThan(QT_MAJOR_VERSION, 5) {
	error("Tetzle requires Qt 5.9 or greater")
}
equals(QT_MAJOR_VERSION, 5):lessThan(QT_MINOR_VERSION, 9) {
	error("Tetzle requires Qt 5.9 or greater")
}

TEMPLATE = app
QT += widgets
macx {
	QMAKE_INFO_PLIST = data/mac/Info.plist
}
CONFIG += c++11

CONFIG(debug, debug|release) {
	CONFIG += warn_on
	DEFINES += QT_DEPRECATED_WARNINGS
	DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x051300
	DEFINES += QT_NO_NARROWING_CONVERSIONS_IN_CONNECT
}

# Allow in-tree builds
MOC_DIR = build
OBJECTS_DIR = build
RCC_DIR = build

# Set program version
VERSION = 2.1.5
DEFINES += VERSIONSTR=\\\"$${VERSION}\\\"

# Set program name
unix: !macx {
	TARGET = tetzle
} else {
	TARGET = Tetzle
}

# Specify program sources
HEADERS = src/add_image.h \
	src/appearance_dialog.h \
	src/board.h \
	src/choose_game_dialog.h \
	src/color_button.h \
	src/dancing_links.h \
	src/generator.h \
	src/graphics_layer.h \
	src/image_properties_dialog.h \
	src/locale_dialog.h \
	src/message.h \
	src/new_game_tab.h \
	src/open_game_tab.h \
	src/overview.h \
	src/path.h \
	src/piece.h \
	src/tile.h \
	src/tag_manager.h \
	src/thumbnail_delegate.h \
	src/thumbnail_loader.h \
	src/toolbar_list.h \
	src/window.h \
	src/zoom_slider.h

SOURCES = src/add_image.cpp \
	src/appearance_dialog.cpp \
	src/board.cpp \
	src/choose_game_dialog.cpp \
	src/color_button.cpp \
	src/dancing_links.cpp \
	src/generator.cpp \
	src/graphics_layer.cpp \
	src/image_properties_dialog.cpp \
	src/locale_dialog.cpp \
	src/main.cpp \
	src/message.cpp \
	src/new_game_tab.cpp \
	src/open_game_tab.cpp \
	src/overview.cpp \
	src/path.cpp \
	src/piece.cpp \
	src/tile.cpp \
	src/tag_manager.cpp \
	src/thumbnail_delegate.cpp \
	src/thumbnail_loader.cpp \
	src/toolbar_list.cpp \
	src/window.cpp \
	src/zoom_slider.cpp

# Generate translations
TRANSLATIONS = $$files(translations/tetzle_*.ts)
qtPrepareTool(LRELEASE, lrelease)
updateqm.input = TRANSLATIONS
updateqm.output = ${QMAKE_FILE_PATH}/${QMAKE_FILE_BASE}.qm
updateqm.commands = $$LRELEASE -silent ${QMAKE_FILE_IN} -qm ${QMAKE_FILE_OUT}
updateqm.CONFIG += no_link target_predeps
QMAKE_EXTRA_COMPILERS += updateqm

# Install program data
RESOURCES = data/data.qrc icons/icon.qrc
macx {
	ICON = icons/tetzle.icns
} else:win32 {
	RC_ICONS = icons/tetzle.ico
	QMAKE_TARGET_DESCRIPTION = "Jigsaw puzzle game"
	QMAKE_TARGET_COPYRIGHT = "Copyright (C) 2019 Graeme Gott"
} else:unix {
	isEmpty(PREFIX) {
		PREFIX = /usr/local
	}
	isEmpty(BINDIR) {
		BINDIR = bin
	}

	target.path = $$PREFIX/$$BINDIR/

	icon.files = icons/hicolor/*
	icon.path = $$PREFIX/share/icons/hicolor/

	pixmap.files = icons/tetzle.xpm
	pixmap.path = $$PREFIX/share/pixmaps/

	desktop.files = data/unix/tetzle.desktop
	desktop.path = $$PREFIX/share/applications/

	appdata.files = data/unix/tetzle.appdata.xml
	appdata.path = $$PREFIX/share/metainfo/

	qm.files = $$replace(TRANSLATIONS, .ts, .qm)
	qm.path = $$PREFIX/share/tetzle/translations/
	qm.CONFIG += no_check_exist

	man.files = doc/tetzle.6
	man.path = $$PREFIX/share/man/man6

	INSTALLS += target icon pixmap desktop appdata qm man
}
