set(TARGET_NAME dde-disk-mount-plugin)

find_package(Qt${GXDE_QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets Concurrent DBus)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GIO REQUIRED IMPORTED_TARGET gio-2.0)
if(GXDE_QT_VERSION_MAJOR GREATER_EQUAL 6)
    pkg_check_modules(DTKWIDGET REQUIRED IMPORTED_TARGET dtk2widget)
    pkg_check_modules(UDISKS2_QT REQUIRED IMPORTED_TARGET udisks2-qt6)
else()
    pkg_check_modules(DTKWIDGET REQUIRED IMPORTED_TARGET dtkwidget)
    pkg_check_modules(UDISKS2_QT REQUIRED IMPORTED_TARGET udisks2-qt5)
endif()

set(LIB_REL ${CMAKE_SOURCE_DIR}/gxde-file-manager-lib)

add_library(${TARGET_NAME} MODULE
    diskmountplugin.cpp
    diskcontrolwidget.cpp
    diskpluginitem.cpp
    diskcontrolitem.cpp
    dattachedudisks2device.cpp
    dattachedvfsdevice.cpp
    diskmountplugin.h
    diskcontrolwidget.h
    diskpluginitem.h
    diskcontrolitem.h
    dattacheddeviceinterface.h
    dattachedudisks2device.h
    dattachedvfsdevice.h

    # 单独编入lib的部分接口（避免依赖整个lib）
    ${LIB_REL}/interfaces/dfmsettings.cpp
    ${LIB_REL}/interfaces/dfmstandardpaths.cpp
    ${LIB_REL}/interfaces/durl.cpp
    ${LIB_REL}/interfaces/dfmsettings.h
    ${LIB_REL}/interfaces/dfmstandardpaths.h
    ${LIB_REL}/interfaces/durl.h

    # vfs.pri
    ${LIB_REL}/interfaces/vfs/dfmvfsmanager.cpp
    ${LIB_REL}/interfaces/vfs/dfmvfsdevice.cpp
    ${LIB_REL}/interfaces/vfs/dfmvfsabstracteventhandler.cpp
    ${LIB_REL}/interfaces/vfs/dfmvfsmanager.h
    ${LIB_REL}/interfaces/vfs/dfmvfsdevice.h
    ${LIB_REL}/interfaces/vfs/dfmvfsabstracteventhandler.h

    resources.qrc
    ${LIB_REL}/configure.qrc
)

target_include_directories(${TARGET_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "/usr/include/gxde-dock"
    "${LIB_REL}/interfaces"
    "${LIB_REL}"
    "${LIB_REL}/interfaces/vfs"
)

target_compile_definitions(${TARGET_NAME} PRIVATE
    QT_MESSAGELOGCONTEXT
    DFM_NO_FILE_WATCHER
)

if(GXDE_QT_VERSION_MAJOR GREATER_EQUAL 6)
    target_compile_definitions(${TARGET_NAME} PRIVATE DFM_USE_QT6)
endif()

target_link_libraries(${TARGET_NAME} PRIVATE
    Qt${GXDE_QT_VERSION_MAJOR}::Core
    Qt${GXDE_QT_VERSION_MAJOR}::Widgets
    Qt${GXDE_QT_VERSION_MAJOR}::Concurrent
    Qt${GXDE_QT_VERSION_MAJOR}::DBus
    PkgConfig::GIO
    PkgConfig::DTKWIDGET
    PkgConfig::UDISKS2_QT
)

set_target_properties(${TARGET_NAME} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/.."
    PREFIX "lib"
)

gxde_compile_translations(${TARGET_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/translations")

install(TARGETS ${TARGET_NAME}
    LIBRARY DESTINATION "${GXDE_PREFIX}/lib/gxde-dock/plugins/system-trays")

install(DIRECTORY translations/
    DESTINATION "${GXDE_PREFIX}/share/${TARGET_NAME}/translations"
    FILES_MATCHING PATTERN "*.qm")
