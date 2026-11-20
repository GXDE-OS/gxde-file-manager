set(TARGET_NAME dde-deb-preview-plugin)

find_package(Qt${GXDE_QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets)

add_library(${TARGET_NAME} MODULE
    main.cpp
    debpreview.cpp
    debpreview.h
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
)

set_target_properties(${TARGET_NAME} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/plugins/previews"
    PREFIX "lib"
)

install(TARGETS ${TARGET_NAME} LIBRARY DESTINATION "${GXDE_PLUGINDIR}/previews")
