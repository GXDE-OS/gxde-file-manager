set(TARGET_NAME gxde-file-manager-daemon)

find_package(Qt${GXDE_QT_VERSION_MAJOR} REQUIRED COMPONENTS Core DBus Concurrent)
find_package(PkgConfig REQUIRED)
pkg_check_modules(X11 REQUIRED IMPORTED_TARGET x11)

if(GXDE_QT_VERSION_MAJOR GREATER_EQUAL 6)
    pkg_check_modules(POLKIT_QT REQUIRED IMPORTED_TARGET polkit-qt6-1)
    pkg_check_modules(DTKWIDGET REQUIRED IMPORTED_TARGET dtk2widget)
else()
    pkg_check_modules(POLKIT_AGENT REQUIRED IMPORTED_TARGET polkit-agent-1)
    pkg_check_modules(POLKIT_QT REQUIRED IMPORTED_TARGET polkit-qt5-1)
    pkg_check_modules(DTKWIDGET REQUIRED IMPORTED_TARGET dtkwidget)
endif()

set(DAEMON_SOURCES
    main.cpp
    app/filemanagerdaemon.cpp
    controllers/appcontroller.cpp
    controllers/fileoperation.cpp
    fileoperationjob/renamejob.cpp
    dbusservice/dbusadaptor/fileoperation_adaptor.cpp
    dbusservice/dbusadaptor/renamejob_adaptor.cpp
    dbusservice/dbusinterface/fileoperation_interface.cpp
    dbusservice/dbusinterface/renamejob_interface.cpp
    client/filemanagerclient.cpp
    dbusservice/dbustype/dbusinforet.cpp
    app/policykithelper.cpp
    fileoperationjob/basejob.cpp
    fileoperationjob/createfolderjob.cpp
    fileoperationjob/createtemplatefilejob.cpp
    fileoperationjob/movejob.cpp
    fileoperationjob/copyjob.cpp
    fileoperationjob/deletejob.cpp
    dbusservice/dbusadaptor/copyjob_adaptor.cpp
    dbusservice/dbusadaptor/createfolderjob_adaptor.cpp
    dbusservice/dbusadaptor/createtemplatefilejob_adaptor.cpp
    dbusservice/dbusadaptor/deletejob_adaptor.cpp
    dbusservice/dbusadaptor/movejob_adaptor.cpp
    dbusservice/dbusinterface/copyjob_interface.cpp
    dbusservice/dbusinterface/createfolderjob_interface.cpp
    dbusservice/dbusinterface/createtemplatefilejob_interface.cpp
    dbusservice/dbusinterface/deletejob_interface.cpp
    dbusservice/dbusinterface/movejob_interface.cpp
    usershare/usersharemanager.cpp
    dbusservice/dbusadaptor/usershare_adaptor.cpp
    dbusservice/dbusinterface/usershare_interface.cpp
    usbformatter/usbformatter.cpp
    dbusservice/dbusadaptor/usbformatter_adaptor.cpp
    dbusservice/dbusinterface/usbformatter_interface.cpp
    commandmanager/commandmanager.cpp
    dbusservice/dbusadaptor/commandmanager_adaptor.cpp
    dbusservice/dbusinterface/commandmanager_interface.cpp
    deviceinfo/deviceinfomanager.cpp
    dbusservice/dbusadaptor/deviceinfomanager_adaptor.cpp
    dbusservice/dbusinterface/deviceinfomanager_interface.cpp
    tag/tagmanagerdaemon.cpp
    dbusservice/dbusadaptor/tagmanagerdaemon_adaptor.cpp
)

# 来自partman.pri
list(APPEND DAEMON_SOURCES
    ${CMAKE_SOURCE_DIR}/partman/command.cpp
    ${CMAKE_SOURCE_DIR}/partman/partitionmanager.cpp
    ${CMAKE_SOURCE_DIR}/partman/partition.cpp
    ${CMAKE_SOURCE_DIR}/partman/string_util.cpp
    ${CMAKE_SOURCE_DIR}/partman/structs.cpp
    ${CMAKE_SOURCE_DIR}/partman/readusagemanager.cpp
)

# movejob_interface.h与renamejob_interface.h 自动生成的 typedef
# 都叫 `daemon::RenameJob` (.xml 里同名)，AUTOMOC 把两个 moc 聚合后撞名。
# 把 movejob_interface.h 排除 AUTOMOC，手动 moc 成独立 TU 即可。
set_source_files_properties(
    "${CMAKE_CURRENT_SOURCE_DIR}/dbusservice/dbusinterface/movejob_interface.h"
    PROPERTIES SKIP_AUTOMOC ON
)
if(GXDE_QT_VERSION_MAJOR GREATER_EQUAL 6)
    qt6_wrap_cpp(DAEMON_EXTRA_MOC
        "${CMAKE_CURRENT_SOURCE_DIR}/dbusservice/dbusinterface/movejob_interface.h")
else()
    qt5_wrap_cpp(DAEMON_EXTRA_MOC
        "${CMAKE_CURRENT_SOURCE_DIR}/dbusservice/dbusinterface/movejob_interface.h")
endif()

add_executable(${TARGET_NAME} ${DAEMON_SOURCES} ${DAEMON_EXTRA_MOC})

target_include_directories(${TARGET_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/dbusservice"
    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/utils"
    "${CMAKE_SOURCE_DIR}/gxde-file-manager-lib"
    "${CMAKE_SOURCE_DIR}/gxde-file-manager-lib/interfaces"
    "${CMAKE_SOURCE_DIR}/gxde-file-manager-lib/shutil"
)

target_compile_definitions(${TARGET_NAME} PRIVATE QT_MESSAGELOGCONTEXT)

if(GXDE_QT_VERSION_MAJOR GREATER_EQUAL 6)
    target_compile_definitions(${TARGET_NAME} PRIVATE DFM_USE_QT6)
endif()

target_link_libraries(${TARGET_NAME} PRIVATE
    gxde-file-manager-lib
    Qt${GXDE_QT_VERSION_MAJOR}::Core
    Qt${GXDE_QT_VERSION_MAJOR}::DBus
    Qt${GXDE_QT_VERSION_MAJOR}::Concurrent
    PkgConfig::X11
    PkgConfig::POLKIT_QT
    PkgConfig::DTKWIDGET
)
if(GXDE_QT_VERSION_MAJOR EQUAL 5 AND TARGET PkgConfig::POLKIT_AGENT)
    target_link_libraries(${TARGET_NAME} PRIVATE PkgConfig::POLKIT_AGENT)
endif()

install(TARGETS ${TARGET_NAME} RUNTIME DESTINATION "/usr/bin")

install(FILES dbusservice/com.deepin.filemanager.daemon.conf
    DESTINATION "/etc/dbus-1/system.d")

install(FILES dbusservice/com.deepin.filemanager.daemon.service
    DESTINATION "/usr/share/dbus-1/system-services")

install(FILES dbusservice/dde-filemanager-daemon.service
    DESTINATION "/usr/lib/systemd/system")
