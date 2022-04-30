cmake_minimum_required(VERSION 3.16.0)

# Configure project
project(tetzle VERSION 2.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui LinguistTools OpenGL OpenGLWidgets Widgets)
if(UNIX AND NOT APPLE)
	include(GNUInstallDirs)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_compile_definitions(
		QT_STRICT_ITERATORS
		QT_NO_NARROWING_CONVERSIONS_IN_CONNECT
		QT_DISABLE_DEPRECATED_BEFORE=0x060300
	)
endif()

# Create translations
file(GLOB translations_SRCS translations/*.ts)
qt6_add_translation(translations_QM ${translations_SRCS})

add_custom_target(update_translations
	COMMAND Qt6::lupdate -no-obsolete -locations none src -ts ${translations_SRCS}
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	VERBATIM
)

# Create program
qt6_add_executable(tetzle
	src/add_image.cpp
	src/appearance_dialog.cpp
	src/board.cpp
	src/choose_game_dialog.cpp
	src/color_button.cpp
	src/dancing_links.cpp
	src/generator.cpp
	src/graphics_layer.cpp
	src/image_properties_dialog.cpp
	src/locale_dialog.cpp
	src/main.cpp
	src/message.cpp
	src/new_game_tab.cpp
	src/open_game_tab.cpp
	src/overview.cpp
	src/path.cpp
	src/piece.cpp
	src/tile.cpp
	src/tag_manager.cpp
	src/thumbnail_delegate.cpp
	src/thumbnail_loader.cpp
	src/toolbar_list.cpp
	src/window.cpp
	src/zoom_slider.cpp
	data/data.qrc
	${translations_QM}
)

target_link_libraries(tetzle PRIVATE
	Qt6::Core
	Qt6::Gui
	Qt6::OpenGL
	Qt6::OpenGLWidgets
	Qt6::Widgets
)

target_compile_definitions(tetzle PRIVATE VERSIONSTR="${PROJECT_VERSION}")

# Install
if(UNIX AND NOT APPLE)
	target_sources(tetzle PRIVATE icons/icon.qrc)

	install(TARGETS tetzle RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
	install(FILES ${translations_QM} DESTINATION ${CMAKE_INSTALL_DATADIR}/tetzle/translations)
	install(FILES doc/tetzle.6 DESTINATION ${CMAKE_INSTALL_MANDIR}/man6 COMPONENT doc)
	install(DIRECTORY icons/hicolor DESTINATION ${CMAKE_INSTALL_DATADIR}/icons)
	install(FILES data/unix/tetzle.appdata.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
	install(FILES data/unix/tetzle.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
elseif(WIN32)
	# Use Qt6 macro until CMake provides something
	# https://bugreports.qt.io/browse/QTBUG-87618
	set_target_properties(tetzle PROPERTIES
		WIN32_EXECUTABLE TRUE
		QT_TARGET_VERSION "${PROJECT_VERSION}"
		QT_TARGET_COMPANY_NAME "Graeme Gott"
		QT_TARGET_DESCRIPTION "Jigsaw puzzle game"
		QT_TARGET_COPYRIGHT "Copyright (C) 2022 Graeme Gott"
		QT_TARGET_PRODUCT_NAME "Tetzle"
		QT_TARGET_RC_ICONS ${CMAKE_SOURCE_DIR}/icons/tetzle.ico
	)
	_qt_internal_generate_win32_rc_file(tetzle)
endif()
