cmake_minimum_required (VERSION 2.8)
project (leechcraft_knowhow)
include (InitLCPlugin OPTIONAL)

include_directories (
	${CMAKE_CURRENT_BINARY_DIR}
	${Boost_INCLUDE_DIR}
	${LEECHCRAFT_INCLUDE_DIR}
	)
set (SRCS
	knowhow.cpp
	xmlsettingsmanager.cpp
	tipdialog.cpp
	)
set (FORMS
	tipdialog.ui
	)

QtWrapUi (UIS_H ${FORMS})

add_library (leechcraft_knowhow SHARED
	${COMPILED_TRANSLATIONS}
	${SRCS}
	${UIS_H}
	)
target_link_libraries (leechcraft_knowhow
	${LEECHCRAFT_LIBRARIES}
	)
install (TARGETS leechcraft_knowhow DESTINATION ${LC_PLUGINS_DEST})
install (FILES knowhowsettings.xml DESTINATION ${LC_SETTINGS_DEST})
install (DIRECTORY share/knowhow DESTINATION ${LC_SHARE_DEST})

FindQtLibs (leechcraft_knowhow Widgets Xml)
