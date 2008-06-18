TEMPLATE = app
QT += opengl xml
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

HEADERS = src/add_image_dialog.h \
	src/board.h \
	src/dancing_links.h \
	src/label_image_dialog.h \
	src/label_manager.h \
	src/new_game_dialog.h \
	src/open_game_dialog.h \
	src/solver.h \
	src/tile.h \
	src/thumbnail.h \
	src/thumbnail_list.h \
	src/window.h

SOURCES = src/add_image_dialog.cpp \
	src/board.cpp \
	src/dancing_links.cpp \
	src/label_image_dialog.cpp \
	src/label_manager.cpp \
	src/main.cpp \
	src/new_game_dialog.cpp \
	src/open_game_dialog.cpp \
	src/solver.cpp \
	src/tile.cpp \
	src/thumbnail.cpp \
	src/thumbnail_list.cpp \
	src/window.cpp

macx {
	ICON = icons/tetzle.icns
} else:unix {
	RESOURCES = icons/icon.qrc
} else:win32 {
	RC_FILE = icons/icon.rc
}

unix: !macx {
	isEmpty(PREFIX) {
		PREFIX = /usr/local
	}

	target.path = $$PREFIX/bin/

	icon.path = $$PREFIX/share/icons/hicolor/48x48/apps
	icon.files = icons/tetzle.png

	desktop.path = $$PREFIX/share/applications/
	desktop.files = icons/tetzle.desktop

	INSTALLS += target icon desktop
}
