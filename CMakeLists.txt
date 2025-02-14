# SPDX-FileCopyrightText: 2021-2025 Graeme Gott <graeme@gottcode.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.16)

# Configure project
project(tetzle VERSION 3.0.2 LANGUAGES CXX)

set(project_copyright "2008-2025 Graeme Gott")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui LinguistTools Widgets)
include(GNUInstallDirs)

add_compile_definitions(
	QT_NO_KEYWORDS
	$<$<CONFIG:DEBUG>:QT_STRICT_ITERATORS>
	$<$<CONFIG:DEBUG>:QT_NO_NARROWING_CONVERSIONS_IN_CONNECT>
	$<$<CONFIG:DEBUG>:QT_DISABLE_DEPRECATED_BEFORE=0x060800>
)

# Version number
include(cmake/AddVersionCompileDefinition.cmake)
add_version_compile_definition(src/main.cpp VERSIONSTR)

# Create program
qt_add_executable(tetzle
	# Headers
	src/add_image.h
	src/appearance_dialog.h
	src/board.h
	src/choose_game_dialog.h
	src/color_button.h
	src/dancing_links.h
	src/edge_scroller.h
	src/fragment_list.h
	src/generator.h
	src/image_properties_dialog.h
	src/locale_dialog.h
	src/message.h
	src/new_game_tab.h
	src/open_game_tab.h
	src/overview.h
	src/path.h
	src/piece.h
	src/tag_manager.h
	src/thumbnail_delegate.h
	src/thumbnail_item.h
	src/thumbnail_loader.h
	src/tile.h
	src/toolbar_list.h
	src/window.h
	src/zoom_slider.h
	# Sources
	src/add_image.cpp
	src/appearance_dialog.cpp
	src/board.cpp
	src/choose_game_dialog.cpp
	src/color_button.cpp
	src/dancing_links.cpp
	src/edge_scroller.cpp
	src/fragment_list.cpp
	src/generator.cpp
	src/image_properties_dialog.cpp
	src/locale_dialog.cpp
	src/main.cpp
	src/message.cpp
	src/new_game_tab.cpp
	src/open_game_tab.cpp
	src/overview.cpp
	src/path.cpp
	src/piece.cpp
	src/tag_manager.cpp
	src/thumbnail_delegate.cpp
	src/thumbnail_loader.cpp
	src/tile.cpp
	src/toolbar_list.cpp
	src/window.cpp
	src/zoom_slider.cpp
	# Resources
	data/data.qrc
	${translations_QM}
)

target_link_libraries(tetzle PRIVATE
	Qt6::Core
	Qt6::Gui
	Qt6::Widgets
)

# Create translations
file(GLOB translations_SRCS translations/*.ts)
qt_add_translations(tetzle
	TS_FILES ${translations_SRCS}
	QM_FILES_OUTPUT_VARIABLE translations_QM
	LUPDATE_OPTIONS -no-obsolete -locations none
)

# Optimize build
option(ENABLE_LINK_TIME_OPTIMIZATION "Enable link time optimization" OFF)
if(ENABLE_LINK_TIME_OPTIMIZATION)
	include(CheckIPOSupported)
	check_ipo_supported(RESULT result)
	if(result)
		set_target_properties(tetzle PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
	endif()
endif()

option(ENABLE_STRIP "Enable automatic stripping of builds" OFF)
if(ENABLE_STRIP)
	add_custom_command(TARGET tetzle
		POST_BUILD
		COMMAND ${CMAKE_STRIP} $<TARGET_FILE:tetzle>
	)
endif()

# Install
if(APPLE)
	set(datadir "../Resources")

	set_target_properties(tetzle PROPERTIES
		OUTPUT_NAME Tetzle
		MACOSX_BUNDLE TRUE
		MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/data/mac/Info.plist.in
	)

	include(cmake/BundleResources.cmake)
	bundle_data(tetzle ${CMAKE_SOURCE_DIR}/icons/tetzle.icns Resources)
	bundle_translations(tetzle "${translations_QM}")
elseif(WIN32)
	set(datadir ".")

	# Use Qt6 macro until CMake provides something
	# https://bugreports.qt.io/browse/QTBUG-87618
	set_target_properties(tetzle PROPERTIES
		OUTPUT_NAME Tetzle
		WIN32_EXECUTABLE TRUE
		QT_TARGET_VERSION "${PROJECT_VERSION}"
		QT_TARGET_COMPANY_NAME "Graeme Gott"
		QT_TARGET_DESCRIPTION "Jigsaw puzzle game"
		QT_TARGET_COPYRIGHT "\\xA9 ${project_copyright}"
		QT_TARGET_PRODUCT_NAME "Tetzle"
		QT_TARGET_RC_ICONS ${CMAKE_SOURCE_DIR}/icons/tetzle.ico
	)
	_qt_internal_generate_win32_rc_file(tetzle)
else()
	file(RELATIVE_PATH datadir ${CMAKE_INSTALL_FULL_BINDIR} ${CMAKE_INSTALL_FULL_DATADIR}/tetzle)

	target_sources(tetzle PRIVATE icons/icon.qrc)

	install(TARGETS tetzle RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
	install(FILES ${translations_QM} DESTINATION ${CMAKE_INSTALL_DATADIR}/tetzle/translations)
	install(FILES doc/tetzle.6 DESTINATION ${CMAKE_INSTALL_MANDIR}/man6 COMPONENT doc)
	install(DIRECTORY icons/hicolor DESTINATION ${CMAKE_INSTALL_DATADIR}/icons)

	include(cmake/TranslateMetainfo.cmake)
	process_and_install_metainfo(PO_DIR ${CMAKE_SOURCE_DIR}/data/unix/po)
endif()

set_property(SOURCE src/main.cpp APPEND PROPERTY COMPILE_DEFINITIONS TETZLE_DATADIR="${datadir}")
