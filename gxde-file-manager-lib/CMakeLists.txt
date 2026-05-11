set(TARGET_NAME gxde-file-manager-lib)
set(LIB_OUTPUT_NAME gxde-file-manager)

find_package(Qt${GXDE_QT_VERSION_MAJOR} REQUIRED COMPONENTS
    Core Gui Svg DBus Concurrent Multimedia Xml Widgets
)
find_package(PkgConfig REQUIRED)

if(GXDE_QT_VERSION_MAJOR EQUAL 5)
    find_package(Qt5 REQUIRED COMPONENTS X11Extras)
endif()

if(GXDE_QT_VERSION_MAJOR GREATER_EQUAL 6)
    find_package(Qt6 REQUIRED COMPONENTS Core5Compat)
endif()

# DTK (Qt6 端只用dtk2widget)
pkg_check_modules(DTKWIDGET REQUIRED IMPORTED_TARGET dtk2widget)

# 公共pkg-config依赖
pkg_check_modules(LIBSECRET REQUIRED IMPORTED_TARGET libsecret-1)
pkg_check_modules(GIO_UNIX REQUIRED IMPORTED_TARGET gio-unix-2.0)
pkg_check_modules(POPPLER_CPP REQUIRED IMPORTED_TARGET poppler-cpp)
pkg_check_modules(DISOMASTER REQUIRED IMPORTED_TARGET disomaster)

if(GXDE_QT_VERSION_MAJOR GREATER_EQUAL 6)
    pkg_check_modules(GSETTINGS_QT REQUIRED IMPORTED_TARGET gsettings-qt6)
    pkg_check_modules(UDISKS2_QT REQUIRED IMPORTED_TARGET udisks2-qt6)
else()
    pkg_check_modules(GSETTINGS_QT REQUIRED IMPORTED_TARGET gsettings-qt)
    pkg_check_modules(UDISKS2_QT REQUIRED IMPORTED_TARGET udisks2-qt5)
    pkg_check_modules(QT5XDG REQUIRED IMPORTED_TARGET Qt5Xdg)
endif()

# Qt6需要手动接入KF6 KCodecs
if(GXDE_QT_VERSION_MAJOR GREATER_EQUAL 6)
    set(KF6_KCODECS_INCLUDE "/usr/include/KF6/KCodecs")
else()
    find_package(KF5Codecs QUIET)
endif()

set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(REPO_ROOT "${CMAKE_SOURCE_DIR}")

set(LIB_SOURCES
    controllers/appcontroller.cpp
    views/dfmpreviewsidebar.cpp
    views/dfmsidebaropticaldevitem.cpp
    views/dtoolbar.cpp
    views/dfileview.cpp
    views/ddetailview.cpp
    views/dicontextbutton.cpp
    views/dstatebutton.cpp
    controllers/filecontroller.cpp
    views/fileitem.cpp
    models/desktopfileinfo.cpp
    models/bookmark.cpp
    models/searchhistory.cpp
    controllers/bookmarkmanager.cpp
    dialogs/dialogmanager.cpp
    controllers/searchhistroymanager.cpp
    views/windowmanager.cpp
    shutil/desktopfile.cpp
    shutil/fileutils.cpp
    shutil/properties.cpp
    views/dfilemanagerwindow.cpp
    views/historystack.cpp
    dialogs/propertydialog.cpp
    views/dhoverbutton.cpp
    views/dfmsidebaritemgroup.cpp
    controllers/trashmanager.cpp
    models/trashfileinfo.cpp
    shutil/mimesappsmanager.cpp
    dialogs/openwithdialog.cpp
    controllers/searchcontroller.cpp
    models/searchfileinfo.cpp
    dialogs/basedialog.cpp
    views/extendview.cpp
    controllers/pathmanager.cpp
    views/ddragwidget.cpp
    shutil/mimetypedisplaymanager.cpp
    views/dstatusbar.cpp
    controllers/subscriber.cpp
    models/dfileselectionmodel.cpp
    dialogs/closealldialogindicator.cpp
    gvfs/gvfsmountclient.cpp
    gvfs/mountaskpassworddialog.cpp
    gvfs/networkmanager.cpp
    gvfs/secretmanager.cpp
    models/networkfileinfo.cpp
    controllers/networkcontroller.cpp
    dialogs/trashpropertydialog.cpp
    controllers/jobcontroller.cpp
    views/computerview.cpp
    shutil/shortcut.cpp
    views/dtabbar.cpp
    views/dfiledialog.cpp
    interfaces/dfiledialoghandle.cpp
    dialogs/shareinfoframe.cpp
    interfaces/dfmstandardpaths.cpp
    interfaces/dfmglobal.cpp
    controllers/sharecontroler.cpp
    models/sharefileinfo.cpp
    interfaces/dfileviewhelper.cpp
    interfaces/diconitemdelegate.cpp
    views/fileviewhelper.cpp
    interfaces/dlistitemdelegate.cpp
    interfaces/dstyleditemdelegate.cpp
    interfaces/durl.cpp
    interfaces/dfilemenu.cpp
    interfaces/dfilesystemmodel.cpp
    app/define.cpp
    interfaces/dabstractfilecontroller.cpp
    interfaces/dabstractfileinfo.cpp
    interfaces/dfilemenumanager.cpp
    interfaces/dfileservices.cpp
    interfaces/dfmevent.cpp
    interfaces/dfileinfo.cpp
    interfaces/dfilesystemwatcher.cpp
    interfaces/dabstractfilewatcher.cpp
    interfaces/dfilewatcher.cpp
    interfaces/dfileproxywatcher.cpp
    app/filesignalmanager.cpp
    plugins/pluginmanager.cpp
    interfaces/dthumbnailprovider.cpp
    views/progressline.cpp
    controllers/avfsfilecontroller.cpp
    models/avfsfileinfo.cpp
    interfaces/dfileiconprovider.cpp
    interfaces/dfilewatchermanager.cpp
    dialogs/computerpropertydialog.cpp
    interfaces/dmimedatabase.cpp
    gvfs/qdrive.cpp
    gvfs/qvolume.cpp
    gvfs/qmount.cpp
    gvfs/gvfsmountmanager.cpp
    gvfs/qdiskinfo.cpp
    interfaces/dfmeventdispatcher.cpp
    interfaces/dfmabstracteventhandler.cpp
    controllers/fileeventprocessor.cpp
    dialogs/previewdialog.cpp
    interfaces/dfmbaseview.cpp
    interfaces/dfmviewmanager.cpp
    views/dfmactionbutton.cpp
    interfaces/plugins/dfmviewplugin.cpp
    interfaces/plugins/dfmviewfactory.cpp
    interfaces/plugins/dfmfactoryloader.cpp
    interfaces/plugins/dfmfilecontrollerplugin.cpp
    interfaces/plugins/dfmfilecontrollerfactory.cpp
    interfaces/plugins/dfmgenericplugin.cpp
    interfaces/plugins/dfmgenericfactory.cpp
    views/filedialogstatusbar.cpp
    interfaces/plugins/dfmfilepreviewplugin.cpp
    interfaces/plugins/dfmfilepreviewfactory.cpp
    interfaces/dfmfilepreview.cpp
    dialogs/filepreviewdialog.cpp
    dialogs/usersharepasswordsettingdialog.cpp
    gvfs/mountsecretdiskaskpassworddialog.cpp
    views/drenamebar.cpp
    shutil/filebatchprocess.cpp
    dialogs/ddesktoprenamedialog.cpp
    dialogs/dmultifilepropertydialog.cpp
    dialogs/movetotrashconflictdialog.cpp
    dialogs/dfmsettingdialog.cpp
    controllers/operatorrevocation.cpp
    views/themeconfig.cpp
    controllers/tagcontroller.cpp
    models/tagfileinfo.cpp
    views/dtagactionwidget.cpp
    views/droundbutton.cpp
    views/dtagedit.cpp
    shutil/dsqlitehandle.cpp
    shutil/danythingmonitorfilter.cpp
    controllers/tagmanagerdaemoncontroller.cpp
    controllers/interface/tagmanagerdaemon_interface.cpp
    interfaces/dfmsettings.cpp
    interfaces/dfmsidebar.cpp
    interfaces/dfmsidebaritem.cpp
    views/dfmsidebaritemseparator.cpp
    views/dfmsidebardefaultitem.cpp
    views/dfmsidebarbookmarkitem.cpp
    views/dfmsidebartrashitem.cpp
    views/dfmsidebardeviceitem.cpp
    views/dfmsidebarnetworkitem.cpp
    views/dfmsidebartagitem.cpp
    interfaces/dfmcrumbbar.cpp
    interfaces/dfmcrumbitem.cpp
    interfaces/dfmcrumbinterface.cpp
    interfaces/plugins/dfmcrumbplugin.cpp
    interfaces/plugins/dfmcrumbfactory.cpp
    interfaces/dfmcrumbmanager.cpp
    controllers/dfmfilecrumbcontroller.cpp
    controllers/dfmcomputercrumbcontroller.cpp
    controllers/dfmbookmarkcrumbcontroller.cpp
    controllers/dfmtagcrumbcontroller.cpp
    controllers/dfmnetworkcrumbcontroller.cpp
    controllers/dfmtrashcrumbcontroller.cpp
    controllers/dfmusersharecrumbcontroller.cpp
    controllers/dfmavfscrumbcontroller.cpp
    controllers/dfmsmbcrumbcontroller.cpp
    controllers/dfmmtpcrumbcontroller.cpp
    views/dfmaddressbar.cpp
    views/dcompleterlistview.cpp
    interfaces/dfmapplication.cpp
    controllers/dfmsearchcrumbcontroller.cpp
    views/dfmheaderview.cpp
    controllers/mountcontroller.cpp
    models/mountfileinfo.cpp
    views/dfmsplitter.cpp
    models/recentfileinfo.cpp
    controllers/recentcontroller.cpp
    controllers/dfmrecentcrumbcontroller.cpp
    views/dfmsidebarrecentitem.cpp
    views/dfmsidebarhomeitem.cpp
    views/dfmsidebarrootitem.cpp
    views/dfmadvancesearchbar.cpp
    shutil/dfmregularexpression.cpp
    models/mergeddesktopfileinfo.cpp
    controllers/dfmmdcrumbcontrooler.cpp
    controllers/mergeddesktopcontroller.cpp
    controllers/masteredmediacontroller.cpp
    controllers/dfmmasteredmediacrumbcontroller.cpp
    models/masteredmediafileinfo.cpp
    views/dfmopticalmediawidget.cpp
    dialogs/burnoptdialog.cpp
)

# ---- sources from external .pri includes ----
list(APPEND LIB_SOURCES
    ${REPO_ROOT}/dialogs/dtaskdialog.cpp
    ${REPO_ROOT}/dialogs/dmoveablewidget.cpp
    ${REPO_ROOT}/dialogs/circleprogressanimatepad.cpp

    ${REPO_ROOT}/utils/utils.cpp
    ${REPO_ROOT}/utils/xutil.cpp
    ${REPO_ROOT}/utils/debugobejct.cpp
    ${REPO_ROOT}/utils/qobjecthelper.cpp

    ${REPO_ROOT}/chinese2pinyin/chinese2pinyin.cpp

    ${REPO_ROOT}/fileoperations/filejob.cpp
    ${REPO_ROOT}/fileoperations/sort.cpp

    ${REPO_ROOT}/usershare/shareinfo.cpp
    ${REPO_ROOT}/usershare/usersharemanager.cpp

    ${REPO_ROOT}/partman/command.cpp
    ${REPO_ROOT}/partman/partitionmanager.cpp
    ${REPO_ROOT}/partman/partition.cpp
    ${REPO_ROOT}/partman/string_util.cpp
    ${REPO_ROOT}/partman/structs.cpp
    ${REPO_ROOT}/partman/readusagemanager.cpp

    deviceinfo/udisklistener.cpp
    deviceinfo/udiskdeviceinfo.cpp

    devicemanagement/dfmudisks2deviceinfo.cpp
    devicemanagement/dfmvfsdeviceinfo.cpp
    devicemanagement/dfmdeviceinfo.cpp
    devicemanagement/dfminvaliddeviceinfo.cpp
    devicemanagement/dfmdevicecontroller.cpp

    dbusinterface/diskmount_interface.cpp
    dbusinterface/dbustype.cpp
    dbusinterface/usershare_interface.cpp
    dbusinterface/deviceinfomanager_interface.cpp
    dbusinterface/commandmanager_interface.cpp
    dbusinterface/startmanager_interface.cpp
    dbusinterface/introspectable_interface.cpp

    tag/tagmanager.cpp
    tag/tagutil.cpp

    io/dlocalfiledevice.cpp
    io/dfileiodeviceproxy.cpp
    io/dfilecopymovejob.cpp
    io/dfilehandler.cpp
    io/dfiledevice.cpp
    io/dlocalfilehandler.cpp
    io/dfilestatisticsjob.cpp
    io/dstorageinfo.cpp
    io/dgiofiledevice.cpp

    interfaces/vfs/dfmvfsmanager.cpp
    interfaces/vfs/dfmvfsdevice.cpp
    interfaces/vfs/dfmvfsabstracteventhandler.cpp
)

if(NOT GXDE_DISABLE_ANYTHING)
    list(APPEND LIB_SOURCES shutil/danythingmonitor.cpp)
endif()

if(GXDE_ARCH STREQUAL "sw_64")
    list(APPEND LIB_SOURCES
        sw_label/filemanagerlibrary.cpp
        sw_label/llsdeepinlabellibrary.cpp
    )
endif()

set(LIB_RESOURCES
    skin/skin.qrc
    skin/dialogs.qrc
    skin/filemanager.qrc
    themes/themes.qrc
    configure.qrc
    ${REPO_ROOT}/chinese2pinyin/data.qrc
)

# 调用 lrelease 生成翻译
file(GLOB LIB_QM_TS "${SRC_DIR}/translations/*.ts")

# qobjecthelper.cpp 在注释中含 Q_OBJECT 字样，跳过 AUTOMOC
set_source_files_properties(
    ${REPO_ROOT}/utils/qobjecthelper.cpp
    PROPERTIES SKIP_AUTOMOC ON
)

# dtabbar.h 含自引用 metatype（signal 中含 Tab*），CMake AUTOMOC 把所有
# moc_*.cpp 聚合到 mocs_compilation.cpp，会与其他 header 里 `class Tab;` 前向
# 声明撞车。绕过：对 dtabbar.h 手动跑 moc，单独编译为独立 TU。
set_source_files_properties(
    ${SRC_DIR}/views/dtabbar.h
    PROPERTIES SKIP_AUTOMOC ON
)
if(GXDE_QT_VERSION_MAJOR GREATER_EQUAL 6)
    qt6_wrap_cpp(LIB_EXTRA_MOC ${SRC_DIR}/views/dtabbar.h)
else()
    qt5_wrap_cpp(LIB_EXTRA_MOC ${SRC_DIR}/views/dtabbar.h)
endif()

add_library(${TARGET_NAME} SHARED
    ${LIB_SOURCES}
    ${LIB_RESOURCES}
    ${LIB_EXTRA_MOC}
)
set_target_properties(${TARGET_NAME} PROPERTIES
    OUTPUT_NAME "${LIB_OUTPUT_NAME}"
    VERSION "${GXDE_VERSION}"
    SOVERSION 1
)

target_compile_definitions(${TARGET_NAME} PRIVATE
    QMAKE_TARGET="${LIB_OUTPUT_NAME}"
    QMAKE_VERSION="${GXDE_VERSION}"
    QMAKE_ORGANIZATION_NAME="deepin"
    QT_MESSAGELOGCONTEXT
    APPSHAREDIR="${GXDE_PREFIX}/share/${LIB_OUTPUT_NAME}"
    ${GXDE_COMMON_DEFINES}
    ${GXDE_ARCH_DEFINES}
)

if(GXDE_QT_VERSION_MAJOR GREATER_EQUAL 6)
    target_compile_definitions(${TARGET_NAME} PRIVATE DFM_USE_QT6)
endif()

if(GXDE_DISABLE_ANYTHING)
    target_compile_definitions(${TARGET_NAME} PRIVATE
        DISABLE_QUICK_SEARCH DISABLE_TAG_SUPPORT)
endif()

if(BUILD_MINIMUM)
    target_compile_definitions(${TARGET_NAME} PRIVATE DFM_MINIMUM DISABLE_FFMEPG)
elseif(DISABLE_FFMPEG)
    target_compile_definitions(${TARGET_NAME} PRIVATE DISABLE_FFMEPG)
endif()

if(GXDE_ARCH STREQUAL "sw_64")
    target_compile_definitions(${TARGET_NAME} PRIVATE SW_LABEL)
    target_compile_options(${TARGET_NAME} PRIVATE -mieee)
endif()

target_include_directories(${TARGET_NAME} PUBLIC
    ${GXDE_COMMON_INCLUDES}
    ${SRC_DIR}
    ${SRC_DIR}/interfaces
    ${SRC_DIR}/interfaces/plugins
    ${SRC_DIR}/io
    ${SRC_DIR}/interfaces/vfs
    ${REPO_ROOT}/chinese2pinyin
)
if(GXDE_QT_VERSION_MAJOR GREATER_EQUAL 6 AND KF6_KCODECS_INCLUDE)
    target_include_directories(${TARGET_NAME} PRIVATE ${KF6_KCODECS_INCLUDE})
endif()

target_link_libraries(${TARGET_NAME} PUBLIC
    Qt${GXDE_QT_VERSION_MAJOR}::Core
    Qt${GXDE_QT_VERSION_MAJOR}::Gui
    Qt${GXDE_QT_VERSION_MAJOR}::Svg
    Qt${GXDE_QT_VERSION_MAJOR}::DBus
    Qt${GXDE_QT_VERSION_MAJOR}::Concurrent
    Qt${GXDE_QT_VERSION_MAJOR}::Multimedia
    Qt${GXDE_QT_VERSION_MAJOR}::Xml
    Qt${GXDE_QT_VERSION_MAJOR}::Widgets
    Qt${GXDE_QT_VERSION_MAJOR}::GuiPrivate
)

if(GXDE_QT_VERSION_MAJOR EQUAL 5)
    target_link_libraries(${TARGET_NAME} PUBLIC
        Qt5::X11Extras
        $<TARGET_NAME_IF_EXISTS:KF5::Codecs>
    )
else()
    target_link_libraries(${TARGET_NAME} PUBLIC Qt6::Core5Compat KF6Codecs)
endif()

target_link_libraries(${TARGET_NAME} PRIVATE
    PkgConfig::DTKWIDGET
    PkgConfig::LIBSECRET
    PkgConfig::GIO_UNIX
    PkgConfig::POPPLER_CPP
    PkgConfig::DISOMASTER
    PkgConfig::GSETTINGS_QT
    PkgConfig::UDISKS2_QT
)
if(GXDE_QT_VERSION_MAJOR EQUAL 5)
    target_link_libraries(${TARGET_NAME} PRIVATE PkgConfig::QT5XDG)
endif()

if(GXDE_USE_JEMALLOC)
    target_link_libraries(${TARGET_NAME} PRIVATE jemalloc)
endif()


# tag.pri 引入 Qt Sql
find_package(Qt${GXDE_QT_VERSION_MAJOR} OPTIONAL_COMPONENTS Sql)
if(TARGET Qt${GXDE_QT_VERSION_MAJOR}::Sql)
    target_link_libraries(${TARGET_NAME} PUBLIC Qt${GXDE_QT_VERSION_MAJOR}::Sql)
endif()

# 翻译生成
gxde_compile_translations(${TARGET_NAME} "${SRC_DIR}/translations")

# ---- 安装规则 ----
install(TARGETS ${TARGET_NAME}
    LIBRARY DESTINATION "${LIB_INSTALL_DIR}"
)

set(INCLUDE_INSTALL_DIR "${GXDE_PREFIX}/include/gxde-file-manager")
install(DIRECTORY interfaces/
    DESTINATION "${INCLUDE_INSTALL_DIR}"
    FILES_MATCHING PATTERN "*.h"
    PATTERN "private" EXCLUDE
    PATTERN "vfs/private" EXCLUDE
)
install(DIRECTORY interfaces/plugins/
    DESTINATION "${INCLUDE_INSTALL_DIR}"
    FILES_MATCHING PATTERN "*.h"
)
install(DIRECTORY interfaces/private/
    DESTINATION "${INCLUDE_INSTALL_DIR}/private"
    FILES_MATCHING PATTERN "*.h"
)
install(DIRECTORY gvfs/
    DESTINATION "${INCLUDE_INSTALL_DIR}/gvfs"
    FILES_MATCHING PATTERN "*.h"
)
install(DIRECTORY
    ${REPO_ROOT}/gxde-file-manager-plugins/plugininterfaces/menu/
    ${REPO_ROOT}/gxde-file-manager-plugins/plugininterfaces/preview/
    ${REPO_ROOT}/gxde-file-manager-plugins/plugininterfaces/view/
    DESTINATION "${INCLUDE_INSTALL_DIR}/gxde-file-manager-plugins"
    FILES_MATCHING PATTERN "*.h"
)

# templates
if(BUILD_MINIMUM)
    install(FILES skin/templates/newTxt.txt
        DESTINATION "${GXDE_APPSHAREDIR}/templates")
else()
    install(FILES
        skin/templates/newDoc.doc
        skin/templates/newExcel.xls
        skin/templates/newPowerPoint.ppt
        skin/templates/newTxt.txt
        DESTINATION "${GXDE_APPSHAREDIR}/templates")
endif()

install(FILES
    mimetypes/archive.mimetype
    mimetypes/text.mimetype
    mimetypes/video.mimetype
    mimetypes/audio.mimetype
    mimetypes/image.mimetype
    mimetypes/executable.mimetype
    mimetypes/backup.mimetype
    DESTINATION "${GXDE_APPSHAREDIR}/mimetypes"
)

install(FILES mimetypeassociations/mimetypeassociations.json
    DESTINATION "${GXDE_APPSHAREDIR}/mimetypeassociations")

install(FILES skin/images/${LIB_OUTPUT_NAME}.svg
    DESTINATION "${GXDE_PREFIX}/share/icons/hicolor/scalable/apps")

install(FILES configure/default-view-states.json
    DESTINATION "${GXDE_APPSHAREDIR}/config")

# 翻译 .qm 安装
install(DIRECTORY translations/
    DESTINATION "${GXDE_APPSHAREDIR}/translations"
    FILES_MATCHING PATTERN "*.qm"
)

# pkgconfig 文件
set(PC_LIBDIR "${LIB_INSTALL_DIR}")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/gxde-file-manager.pc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/gxde-file-manager.pc"
    @ONLY
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/gxde-file-manager.pc"
    DESTINATION "${LIB_INSTALL_DIR}/pkgconfig")
