set(TARGET_NAME dde-pdf-preview-plugin)

find_package(Qt${GXDE_QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Concurrent)
find_package(PkgConfig REQUIRED)
pkg_check_modules(POPPLER_CPP REQUIRED IMPORTED_TARGET poppler-cpp)
if(GXDE_QT_VERSION_MAJOR GREATER_EQUAL 6)
    pkg_check_modules(DTKWIDGET REQUIRED IMPORTED_TARGET dtk2widget)
else()
    pkg_check_modules(DTKWIDGET REQUIRED IMPORTED_TARGET dtkwidget)
endif()

add_library(${TARGET_NAME} MODULE
    pdfwidget.cpp
    main.cpp
    pdfpreview.cpp
    pdfwidget.h
    pdfpreview.h
    theme.qrc
)

target_include_directories(${TARGET_NAME} PRIVATE ${GXDE_COMMON_INCLUDES})

if(GXDE_QT_VERSION_MAJOR GREATER_EQUAL 6)
    target_compile_definitions(${TARGET_NAME} PRIVATE DFM_USE_QT6)
endif()

target_link_libraries(${TARGET_NAME} PRIVATE
    gxde-file-manager-lib
    Qt${GXDE_QT_VERSION_MAJOR}::Core
    Qt${GXDE_QT_VERSION_MAJOR}::Gui
    Qt${GXDE_QT_VERSION_MAJOR}::Widgets
    Qt${GXDE_QT_VERSION_MAJOR}::Concurrent
    PkgConfig::POPPLER_CPP
    PkgConfig::DTKWIDGET
)

set_target_properties(${TARGET_NAME} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/plugins/previews"
    PREFIX "lib"
)

install(TARGETS ${TARGET_NAME} LIBRARY DESTINATION "${GXDE_PLUGINDIR}/previews")
