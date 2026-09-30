set(TARGET_NAME ofd)

find_package(Qt${GXDE_QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui)
find_package(PkgConfig REQUIRED)
pkg_check_modules(CAIRO REQUIRED IMPORTED_TARGET cairo)
find_library(ROFD_FFI_LIB NAMES rofd_ffi REQUIRED)

add_executable(${TARGET_NAME} main.cpp)

set_target_properties(${TARGET_NAME} PROPERTIES
    OUTPUT_NAME "ofd"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/tools/thumbnail"
)

target_link_libraries(${TARGET_NAME} PRIVATE
    Qt${GXDE_QT_VERSION_MAJOR}::Core
    Qt${GXDE_QT_VERSION_MAJOR}::Gui
    PkgConfig::CAIRO
    ${ROFD_FFI_LIB}
)

# 与 qmake common.pri 中 TOOLDIR 等价
install(TARGETS ${TARGET_NAME}
    RUNTIME DESTINATION "${GXDE_LIB_BASE_DIR}/tools/thumbnail")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/ofd.json"
    DESTINATION "${GXDE_LIB_BASE_DIR}/tools/thumbnail")
