set(TARGET_NAME gxde-desktop-panel)

find_package(Qt${GXDE_QT_VERSION_MAJOR} REQUIRED COMPONENTS
    Core Gui Widgets Svg DBus Network Concurrent
    MultimediaWidgets Multimedia
)
if(GXDE_QT_VERSION_MAJOR EQUAL 5)
    find_package(Qt5 REQUIRED COMPONENTS X11Extras)
endif()
if(GXDE_QT_VERSION_MAJOR GREATER_EQUAL 6)
    find_package(Qt6 REQUIRED COMPONENTS Core5Compat)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(XCB REQUIRED IMPORTED_TARGET xcb)
pkg_check_modules(XCB_EWMH REQUIRED IMPORTED_TARGET xcb-ewmh)
pkg_check_modules(XCB_SHAPE REQUIRED IMPORTED_TARGET xcb-shape)
pkg_check_modules(DFRAMEWORKDBUS REQUIRED IMPORTED_TARGET dframeworkdbus)
if(GXDE_QT_VERSION_MAJOR GREATER_EQUAL 6)
    pkg_check_modules(GSETTINGS_QT REQUIRED IMPORTED_TARGET gsettings-qt6)
    pkg_check_modules(DTKWIDGET REQUIRED IMPORTED_TARGET dtk2widget)
else()
    pkg_check_modules(GSETTINGS_QT REQUIRED IMPORTED_TARGET gsettings-qt)
    pkg_check_modules(DTKWIDGET REQUIRED IMPORTED_TARGET dtkwidget)
endif()

set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

# 主源文件
set(PANEL_SOURCES
    main.cpp
    config/config.cpp
    desktop.cpp
    view/canvasviewhelper.cpp
    model/dfileselectionmodel.cpp
    view/canvasgridview.cpp
    presenter/apppresenter.cpp
    presenter/gridmanager.cpp
    dbus/dbusdisplay.cpp
    presenter/display.cpp
    presenter/dfmsocketinterface.cpp
    view/watermaskframe.cpp
    dbus/dbusdock.cpp
    config/profile.cpp
    view/desktopitemdelegate.cpp
    presenter/gridcore.cpp
    config/dfmdesktopsettings.cpp
    view/backgroundhelper.cpp
)
set(PANEL_HEADERS
    config/config.h
    desktop.h
    view/canvasviewhelper.h
    model/dfileselectionmodel.h
    view/private/canvasviewprivate.h
    global/coorinate.h
    global/singleton.h
    view/canvasgridview.h
    presenter/apppresenter.h
    presenter/gridmanager.h
    dbus/dbusdisplay.h
    presenter/display.h
    presenter/dfmsocketinterface.h
    view/watermaskframe.h
    dbus/dbusdock.h
    config/profile.h
    view/desktopitemdelegate.h
    presenter/gridcore.h
    deventfilter.h
    config/dfmdesktopsettings.h
    view/backgroundhelper.h
)

# util.pri
list(APPEND PANEL_SOURCES
    util/dde/ddesession.cpp
    util/xcb/xcb.cpp
    util/util.cpp
)
list(APPEND PANEL_HEADERS
    util/dde/ddesession.h
    util/xcb/xcb.h
    util/util.h
)

# dbus.pri (filedialog + filemanager1)
list(APPEND PANEL_SOURCES
    dbus/filedialog/dbusfiledialoghandle.cpp
    dbus/filedialog/dbusfiledialogmanager.cpp
    dbus/filemanager1/dbusfilemanager1.cpp
)
list(APPEND PANEL_HEADERS
    dbus/filedialog/dbusfiledialoghandle.h
    dbus/filedialog/dbusfiledialogmanager.h
    dbus/filemanager1/dbusfilemanager1.h
)

# gxde-wallpaper-chooser.pri
list(APPEND PANEL_SOURCES
    ${CMAKE_SOURCE_DIR}/gxde-wallpaper-chooser/frame.cpp
    ${CMAKE_SOURCE_DIR}/gxde-wallpaper-chooser/wallpaperlist.cpp
    ${CMAKE_SOURCE_DIR}/gxde-wallpaper-chooser/wallpaperitem.cpp
    ${CMAKE_SOURCE_DIR}/gxde-wallpaper-chooser/thumbnailmanager.cpp
    ${CMAKE_SOURCE_DIR}/gxde-wallpaper-chooser/button.cpp
    ${CMAKE_SOURCE_DIR}/gxde-wallpaper-chooser/dbus/deepin_wm.cpp
)
list(APPEND PANEL_HEADERS
    ${CMAKE_SOURCE_DIR}/gxde-wallpaper-chooser/frame.h
    ${CMAKE_SOURCE_DIR}/gxde-wallpaper-chooser/wallpaperlist.h
    ${CMAKE_SOURCE_DIR}/gxde-wallpaper-chooser/wallpaperitem.h
    ${CMAKE_SOURCE_DIR}/gxde-wallpaper-chooser/constants.h
    ${CMAKE_SOURCE_DIR}/gxde-wallpaper-chooser/thumbnailmanager.h
    ${CMAKE_SOURCE_DIR}/gxde-wallpaper-chooser/button.h
    ${CMAKE_SOURCE_DIR}/gxde-wallpaper-chooser/dbus/deepin_wm.h
)

# gxde-zone.pri
list(APPEND PANEL_SOURCES
    ${CMAKE_SOURCE_DIR}/gxde-zone/hotzone.cpp
    ${CMAKE_SOURCE_DIR}/gxde-zone/hoverwidget.cpp
    ${CMAKE_SOURCE_DIR}/gxde-zone/dbus/dbuszone.cpp
    ${CMAKE_SOURCE_DIR}/gxde-zone/pushbuttonlist.cpp
    ${CMAKE_SOURCE_DIR}/gxde-zone/mainwindow.cpp
)
list(APPEND PANEL_HEADERS
    ${CMAKE_SOURCE_DIR}/gxde-zone/hotzone.h
    ${CMAKE_SOURCE_DIR}/gxde-zone/hoverwidget.h
    ${CMAKE_SOURCE_DIR}/gxde-zone/dbus/dbuszone.h
    ${CMAKE_SOURCE_DIR}/gxde-zone/pushbuttonlist.h
    ${CMAKE_SOURCE_DIR}/gxde-zone/mainwindow.h
)

# 资源
set(PANEL_RESOURCES
    resource/theme/theme.qrc
    ${CMAKE_SOURCE_DIR}/gxde-wallpaper-chooser/images.qrc
    ${CMAKE_SOURCE_DIR}/gxde-zone/image.qrc
)

# 生成 DBus adaptors
if(GXDE_QT_VERSION_MAJOR GREATER_EQUAL 6)
    qt6_add_dbus_adaptor(PANEL_DBUS_ADAPTORS
        "${SRC_DIR}/dbus/filedialog/com.deepin.filemanager.filedialog.xml"
        "${SRC_DIR}/dbus/filedialog/dbusfiledialoghandle.h"
        "DBusFileDialogHandle" "filedialog_adaptor")
    qt6_add_dbus_adaptor(PANEL_DBUS_ADAPTORS
        "${SRC_DIR}/dbus/filedialog/com.deepin.filemanager.filedialogmanager.xml"
        "${SRC_DIR}/dbus/filedialog/dbusfiledialogmanager.h"
        "DBusFileDialogManager" "filedialogmanager_adaptor")
    qt6_add_dbus_adaptor(PANEL_DBUS_ADAPTORS
        "${SRC_DIR}/dbus/filemanager1/org.freedesktop.FileManager1.xml"
        "${SRC_DIR}/dbus/filemanager1/dbusfilemanager1.h"
        "DBusFileManager1" "filemanager1_adaptor")
else()
    qt5_add_dbus_adaptor(PANEL_DBUS_ADAPTORS
        "${SRC_DIR}/dbus/filedialog/com.deepin.filemanager.filedialog.xml"
        "${SRC_DIR}/dbus/filedialog/dbusfiledialoghandle.h"
        "DBusFileDialogHandle" "filedialog_adaptor")
    qt5_add_dbus_adaptor(PANEL_DBUS_ADAPTORS
        "${SRC_DIR}/dbus/filedialog/com.deepin.filemanager.filedialogmanager.xml"
        "${SRC_DIR}/dbus/filedialog/dbusfiledialogmanager.h"
        "DBusFileDialogManager" "filedialogmanager_adaptor")
    qt5_add_dbus_adaptor(PANEL_DBUS_ADAPTORS
        "${SRC_DIR}/dbus/filemanager1/org.freedesktop.FileManager1.xml"
        "${SRC_DIR}/dbus/filemanager1/dbusfilemanager1.h"
        "DBusFileManager1" "filemanager1_adaptor")
endif()

# 来自 gxde-wallpaper-chooser.pri 的 DBus interfaces
# qmake 默认按 <basename>_interface 命名（screensaver_interface.h），
# CMake 的 qt_add_dbus_interfaces 默认拼成 screensaverinterface.h。
# 用单数版本 qt_add_dbus_interface 指定 basename 与 qmake 对齐。
if(GXDE_QT_VERSION_MAJOR GREATER_EQUAL 6)
    qt6_add_dbus_interface(PANEL_DBUS_IFACES
        "${CMAKE_SOURCE_DIR}/gxde-wallpaper-chooser/dbus/com.deepin.ScreenSaver.xml"
        screensaver_interface)
    qt6_add_dbus_interface(PANEL_DBUS_IFACES
        "${CMAKE_SOURCE_DIR}/gxde-wallpaper-chooser/dbus/com.deepin.daemon.Appearance.xml"
        appearance_interface)
else()
    qt5_add_dbus_interface(PANEL_DBUS_IFACES
        "${CMAKE_SOURCE_DIR}/gxde-wallpaper-chooser/dbus/com.deepin.ScreenSaver.xml"
        screensaver_interface)
    qt5_add_dbus_interface(PANEL_DBUS_IFACES
        "${CMAKE_SOURCE_DIR}/gxde-wallpaper-chooser/dbus/com.deepin.daemon.Appearance.xml"
        appearance_interface)
endif()

add_executable(${TARGET_NAME}
    ${PANEL_SOURCES}
    ${PANEL_HEADERS}
    ${PANEL_RESOURCES}
    ${PANEL_DBUS_ADAPTORS}
    ${PANEL_DBUS_IFACES}
)

target_include_directories(${TARGET_NAME} PRIVATE
    "${SRC_DIR}"
    "${SRC_DIR}/dbus/filedialog"
    "${SRC_DIR}/dbus/filemanager1"
    "${SRC_DIR}/view"
    "${CMAKE_SOURCE_DIR}/gxde-file-manager-lib"
    "${CMAKE_SOURCE_DIR}/gxde-file-manager-lib/interfaces"
    "${CMAKE_SOURCE_DIR}/utils"
    "${CMAKE_SOURCE_DIR}/gxde-wallpaper-chooser"
    "${CMAKE_SOURCE_DIR}/gxde-wallpaper-chooser/view"
    "${CMAKE_SOURCE_DIR}/gxde-zone"
    "${CMAKE_BINARY_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}"  # 自动生成的 DBus adaptor/interface 头放这里
)

target_compile_definitions(${TARGET_NAME} PRIVATE
    QT_MESSAGELOGCONTEXT
    DDE_DBUS_DISPLAY
    PRO_FILE_PWD="${SRC_DIR}"
)
if(GXDE_ARCH MATCHES "^(sw_64|mips64|mips32)$")
    target_compile_definitions(${TARGET_NAME} PRIVATE DDE_COMPUTER_TRASH)
endif()
if(GXDE_QT_VERSION_MAJOR GREATER_EQUAL 6)
    target_compile_definitions(${TARGET_NAME} PRIVATE DFM_USE_QT6)
endif()

target_link_libraries(${TARGET_NAME} PRIVATE
    gxde-file-manager-lib
    Qt${GXDE_QT_VERSION_MAJOR}::Core
    Qt${GXDE_QT_VERSION_MAJOR}::Gui
    Qt${GXDE_QT_VERSION_MAJOR}::Widgets
    Qt${GXDE_QT_VERSION_MAJOR}::Svg
    Qt${GXDE_QT_VERSION_MAJOR}::DBus
    Qt${GXDE_QT_VERSION_MAJOR}::Network
    Qt${GXDE_QT_VERSION_MAJOR}::Concurrent
    Qt${GXDE_QT_VERSION_MAJOR}::MultimediaWidgets
    Qt${GXDE_QT_VERSION_MAJOR}::Multimedia
    Qt${GXDE_QT_VERSION_MAJOR}::GuiPrivate
    PkgConfig::XCB
    PkgConfig::XCB_EWMH
    PkgConfig::XCB_SHAPE
    PkgConfig::DFRAMEWORKDBUS
    PkgConfig::GSETTINGS_QT
    PkgConfig::DTKWIDGET
)
if(GXDE_QT_VERSION_MAJOR EQUAL 5)
    target_link_libraries(${TARGET_NAME} PRIVATE Qt5::X11Extras)
endif()
if(GXDE_QT_VERSION_MAJOR GREATER_EQUAL 6)
    target_link_libraries(${TARGET_NAME} PRIVATE Qt6::Core5Compat)
endif()

# git rev → GIT_VERSION
find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(GIT_VERSION)
        target_compile_definitions(${TARGET_NAME} PRIVATE GIT_VERSION="${GIT_VERSION}")
    endif()
endif()

gxde_compile_translations(${TARGET_NAME} "${SRC_DIR}/translations")

install(TARGETS ${TARGET_NAME} RUNTIME DESTINATION "${GXDE_PREFIX}/bin")

install(DIRECTORY data/applications/
    DESTINATION "${GXDE_PREFIX}/share/applications"
    FILES_MATCHING PATTERN "*")

install(FILES data/com.deepin.dde.desktop.service
    DESTINATION "${GXDE_PREFIX}/share/dbus-1/services")

install(DIRECTORY translations/
    DESTINATION "${GXDE_PREFIX}/share/${TARGET_NAME}/translations"
    FILES_MATCHING PATTERN "*.qm")

# filedialog dbus xml + service
install(FILES
    dbus/filedialog/com.deepin.filemanager.filedialog.xml
    dbus/filedialog/com.deepin.filemanager.filedialogmanager.xml
    DESTINATION "/usr/share/dbus-1/interfaces")
install(FILES dbus/filedialog/com.deepin.filemanager.filedialog.service
    DESTINATION "/usr/share/dbus-1/services")

# filemanager1 service
install(FILES dbus/filemanager1/org.freedesktop.FileManager.service
    DESTINATION "/usr/share/dbus-1/services")
