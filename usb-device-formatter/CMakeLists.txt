set(TARGET_NAME usb-device-formatter)

find_package(Qt${GXDE_QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Concurrent Network Widgets DBus)
if(GXDE_QT_VERSION_MAJOR EQUAL 5)
    find_package(Qt5 REQUIRED COMPONENTS X11Extras)
endif()
find_package(PkgConfig REQUIRED)
pkg_check_modules(X11 REQUIRED IMPORTED_TARGET x11)
if(GXDE_QT_VERSION_MAJOR GREATER_EQUAL 6)
    pkg_check_modules(DTKWIDGET REQUIRED IMPORTED_TARGET dtk2widget)
else()
    pkg_check_modules(DTKWIDGET REQUIRED IMPORTED_TARGET dtkwidget)
endif()

set(FORMATTER_SOURCES
    main.cpp
    view/mainwindow.cpp
    widgets/progressline.cpp
    view/mainpage.cpp
    view/warnpage.cpp
    view/formatingpage.cpp
    widgets/progressbox.cpp
    view/finishpage.cpp
    view/errorpage.cpp
    app/cmdmanager.cpp
    dialogs/messagedialog.cpp
    app/singletonapp.cpp

    view/mainwindow.h
    widgets/progressline.h
    view/mainpage.h
    view/warnpage.h
    view/formatingpage.h
    widgets/progressbox.h
    view/finishpage.h
    view/errorpage.h
    app/cmdmanager.h
    dialogs/messagedialog.h
    app/singletonapp.h

    theme/theme.qrc
)

# partman.pri
list(APPEND FORMATTER_SOURCES
    ${CMAKE_SOURCE_DIR}/partman/command.cpp
    ${CMAKE_SOURCE_DIR}/partman/partitionmanager.cpp
    ${CMAKE_SOURCE_DIR}/partman/partition.cpp
    ${CMAKE_SOURCE_DIR}/partman/string_util.cpp
    ${CMAKE_SOURCE_DIR}/partman/structs.cpp
    ${CMAKE_SOURCE_DIR}/partman/readusagemanager.cpp
)

add_executable(${TARGET_NAME} ${FORMATTER_SOURCES})

target_include_directories(${TARGET_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/utils"
    "${CMAKE_SOURCE_DIR}"
)

if(GXDE_QT_VERSION_MAJOR GREATER_EQUAL 6)
    target_compile_definitions(${TARGET_NAME} PRIVATE DFM_USE_QT6)
endif()

target_link_libraries(${TARGET_NAME} PRIVATE
    Qt${GXDE_QT_VERSION_MAJOR}::Core
    Qt${GXDE_QT_VERSION_MAJOR}::Gui
    Qt${GXDE_QT_VERSION_MAJOR}::GuiPrivate
    Qt${GXDE_QT_VERSION_MAJOR}::Concurrent
    Qt${GXDE_QT_VERSION_MAJOR}::Network
    Qt${GXDE_QT_VERSION_MAJOR}::Widgets
    Qt${GXDE_QT_VERSION_MAJOR}::DBus
    PkgConfig::X11
    PkgConfig::DTKWIDGET
)
if(GXDE_QT_VERSION_MAJOR EQUAL 5)
    target_link_libraries(${TARGET_NAME} PRIVATE Qt5::X11Extras)
endif()

gxde_compile_translations(${TARGET_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/translations")

set(_FMT_PREFIX "${GXDE_PREFIX}")
install(TARGETS ${TARGET_NAME} RUNTIME DESTINATION "${_FMT_PREFIX}/bin")

install(DIRECTORY translations/
    DESTINATION "${_FMT_PREFIX}/share/${TARGET_NAME}/translations"
    FILES_MATCHING PATTERN "*.qm")

install(FILES pkexec/com.deepin.pkexec.usb-device-formatter.policy
    DESTINATION "${_FMT_PREFIX}/share/polkit-1/actions")

install(PROGRAMS pkexec/usb-device-formatter-pkexec
    DESTINATION "/usr/bin")
