set(TARGET_NAME gxde-file-manager-app)
set(APP_OUTPUT_NAME gxde-file-manager)

find_package(Qt${GXDE_QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Network)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GIO_UNIX REQUIRED IMPORTED_TARGET gio-unix-2.0)

if(GXDE_QT_VERSION_MAJOR GREATER_EQUAL 6)
    find_package(Qt6 REQUIRED COMPONENTS Core5Compat)
    pkg_check_modules(DTKWIDGET REQUIRED IMPORTED_TARGET dtk2widget)
else()
    pkg_check_modules(DTKWIDGET REQUIRED IMPORTED_TARGET dtkwidget)
endif()

add_executable(${TARGET_NAME}
    main.cpp
    filemanagerapp.cpp
    logutil.cpp
    singleapplication.cpp
    commandlinemanager.cpp
    filemanagerapp.h
    logutil.h
    singleapplication.h
    commandlinemanager.h
)

target_include_directories(${TARGET_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/gxde-file-manager-lib"
    "${CMAKE_SOURCE_DIR}/gxde-file-manager-lib/interfaces"
    "${CMAKE_SOURCE_DIR}/utils"
    "${CMAKE_SOURCE_DIR}"
)

target_compile_definitions(${TARGET_NAME} PRIVATE
    QMAKE_TARGET="${APP_OUTPUT_NAME}"
    QMAKE_VERSION="${GXDE_VERSION}"
    QMAKE_ORGANIZATION_NAME="deepin"
    QT_MESSAGELOGCONTEXT
    APPSHAREDIR="${GXDE_PREFIX}/share/${APP_OUTPUT_NAME}"
)

if(GXDE_QT_VERSION_MAJOR GREATER_EQUAL 6)
    target_compile_definitions(${TARGET_NAME} PRIVATE DFM_USE_QT6)
endif()

target_link_libraries(${TARGET_NAME} PRIVATE
    gxde-file-manager-lib  # CMake target name of the shared lib

    Qt${GXDE_QT_VERSION_MAJOR}::Core
    Qt${GXDE_QT_VERSION_MAJOR}::Gui
    Qt${GXDE_QT_VERSION_MAJOR}::Widgets
    Qt${GXDE_QT_VERSION_MAJOR}::Network
    PkgConfig::DTKWIDGET
    PkgConfig::GIO_UNIX
)
if(GXDE_QT_VERSION_MAJOR GREATER_EQUAL 6)
    target_link_libraries(${TARGET_NAME} PRIVATE Qt6::Core5Compat)
endif()

set_target_properties(${TARGET_NAME} PROPERTIES OUTPUT_NAME "${APP_OUTPUT_NAME}")

gxde_compile_translations(${TARGET_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/translations")

install(TARGETS ${TARGET_NAME} RUNTIME DESTINATION "${GXDE_PREFIX}/bin")

if(GXDE_ARCH MATCHES "^(sw_64|mips64|mips32)$")
    install(FILES mips/${APP_OUTPUT_NAME}.desktop
        DESTINATION "${GXDE_PREFIX}/share/applications")
else()
    install(FILES ${APP_OUTPUT_NAME}.desktop
        DESTINATION "${GXDE_PREFIX}/share/applications")
endif()

install(PROGRAMS gxde-xdg-user-dirs-update
    DESTINATION "${GXDE_PREFIX}/bin")

install(FILES pkexec/com.deepin.pkexec.gxde-file-manager.policy
    DESTINATION "${GXDE_PREFIX}/share/polkit-1/actions")

install(PROGRAMS pkexec/gxde-file-manager-pkexec
    DESTINATION "/usr/bin")

install(PROGRAMS gxde-property-dialog
    DESTINATION "/usr/bin")

install(DIRECTORY translations/
    DESTINATION "${GXDE_APPSHAREDIR}/translations"
    FILES_MATCHING PATTERN "*.qm")

if(GXDE_ARCH MATCHES "^(sw_64|mips64|mips32)$")
    install(PROGRAMS
        mips/gxde-computer.sh
        mips/gxde-trash.sh
        mips/file-manager.sh
        DESTINATION "${GXDE_PREFIX}/bin")
    install(FILES
        mips/gxde-file-manager-autostart.desktop
        gxde-file-manager-dialog-autostart.desktop
        DESTINATION "/etc/xdg/autostart")
else()
    install(FILES
        gxde-file-manager-xdg-autostart.desktop
        gxde-file-manager-dialog-autostart.desktop
        DESTINATION "/etc/xdg/autostart")
endif()
