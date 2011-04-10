TEMPLATE = app
QT += opengl
CONFIG += warn_on release
macx {
	# Uncomment the following line to compile on PowerPC Macs
	# QMAKE_MAC_SDK = /Developer/SDKs/MacOSX10.4u.sdk
	CONFIG += x86 ppc
}

MOC_DIR = build
OBJECTS_DIR = build
RCC_DIR = build

unix: !macx {
	TARGET = tetzle
} else {
	TARGET = Tetzle
}

HEADERS = src/add_image.h \
	src/appearance_dialog.h \
	src/board.h \
	src/choose_game_dialog.h \
	src/color_button.h \
	src/dancing_links.h \
	src/generator.h \
	src/graphics_layer.h \
	src/locale_dialog.h \
	src/message.h \
	src/new_game_tab.h \
	src/open_game_tab.h \
	src/overview.h \
	src/path.h \
	src/piece.h \
	src/tile.h \
	src/tag_image_dialog.h \
	src/tag_manager.h \
	src/thumbnail_loader.h \
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
	src/locale_dialog.cpp \
	src/main.cpp \
	src/message.cpp \
	src/new_game_tab.cpp \
	src/open_game_tab.cpp \
	src/overview.cpp \
	src/path.cpp \
	src/piece.cpp \
	src/tile.cpp \
	src/tag_image_dialog.cpp \
	src/tag_manager.cpp \
	src/thumbnail_loader.cpp \
	src/window.cpp \
	src/zoom_slider.cpp

TRANSLATIONS = translations/en.ts

RESOURCES = data/data.qrc icons/icon.qrc
macx:ICON = icons/tetzle.icns
win32:RC_FILE = icons/icon.rc

unix: !macx {
	isEmpty(PREFIX) {
		PREFIX = /usr/local
	}
	isEmpty(BINDIR) {
		BINDIR = bin
	}

	target.path = $$PREFIX/$$BINDIR/

	icon.files = icons/hicolor/*
	icon.path = $$PREFIX/share/icons/hicolor/

	desktop.files = icons/tetzle.desktop
	desktop.path = $$PREFIX/share/applications/

	INSTALLS += target icon desktop
}
