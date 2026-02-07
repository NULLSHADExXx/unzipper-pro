cmake_minimum_required(VERSION 3.16)
project(UnzipperPro VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS 
    Core 
    Gui 
    Widgets 
    Concurrent 
    REQUIRED
)

# Cross-platform libarchive detection
if(WIN32)
    find_package(LibArchive REQUIRED)
    set(LIBARCHIVE_LIBRARIES LibArchive::LibArchive)
    set(LIBARCHIVE_INCLUDE_DIRS "")
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBARCHIVE REQUIRED libarchive)
endif()

set(PROJECT_SOURCES
    src/main.cpp
    src/mainwindow.h
    src/mainwindow_pro.cpp
    src/settingsdialog.h
    src/settingsdialog.cpp
    src/extractionworker.h
    src/extractionworker.cpp
)

qt_add_resources(PROJECT_SOURCES resources/resources.qrc)

add_executable(UnzipperPro ${PROJECT_SOURCES})

target_link_libraries(UnzipperPro
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Concurrent
    ${LIBARCHIVE_LIBRARIES}
)

if(NOT WIN32)
    target_include_directories(UnzipperPro PRIVATE
        ${LIBARCHIVE_INCLUDE_DIRS}
    )
endif()

# Set application properties
set_target_properties(UnzipperPro PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.unzipper.pro"
    MACOSX_BUNDLE_BUNDLE_NAME "Unzipper Pro"
    MACOSX_BUNDLE_BUNDLE_VERSION "2.0.0"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "2.0"
    MACOSX_BUNDLE_ICON_FILE "AppIcon.icns"
)
target_sources(UnzipperPro PRIVATE ${CMAKE_SOURCE_DIR}/AppIcon.icns)
if(APPLE)
    set_source_files_properties(${CMAKE_SOURCE_DIR}/AppIcon.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    add_custom_command(TARGET UnzipperPro POST_BUILD
        COMMAND /usr/libexec/PlistBuddy -c "Set :CFBundleIconFile AppIcon" "$<TARGET_BUNDLE_DIR:UnzipperPro>/Contents/Info.plist"
        COMMAND codesign --force --deep -s - "$<TARGET_BUNDLE_DIR:UnzipperPro>"
    )
endif()

# Installation
install(TARGETS UnzipperPro
    BUNDLE DESTINATION . COMPONENT Runtime
    RUNTIME DESTINATION bin COMPONENT Runtime
)
