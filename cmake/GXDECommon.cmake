# GXDECommon.cmake
# 公共构建设置，对应 common/common.pri

if(NOT DEFINED GXDE_PROJECT_NAME)
    set(GXDE_PROJECT_NAME "gxde-file-manager")
endif()

if(NOT DEFINED CMAKE_INSTALL_PREFIX OR CMAKE_INSTALL_PREFIX STREQUAL "")
    set(CMAKE_INSTALL_PREFIX "/usr" CACHE PATH "" FORCE)
endif()
set(GXDE_PREFIX "${CMAKE_INSTALL_PREFIX}")
set(GXDE_APPSHAREDIR "${GXDE_PREFIX}/share/${GXDE_PROJECT_NAME}")

include(GNUInstallDirs)

if(NOT DEFINED LIB_INSTALL_DIR OR LIB_INSTALL_DIR STREQUAL "")
    set(LIB_INSTALL_DIR "${CMAKE_INSTALL_FULL_LIBDIR}")
endif()
set(GXDE_LIB_BASE_DIR "${LIB_INSTALL_DIR}/${GXDE_PROJECT_NAME}")
set(GXDE_PLUGINDIR "${GXDE_LIB_BASE_DIR}/plugins")
set(GXDE_TOOLDIR "${GXDE_LIB_BASE_DIR}/tools")

if(NOT DEFINED GXDE_VERSION OR GXDE_VERSION STREQUAL "")
    set(GXDE_VERSION "1.8.2")
endif()

set(GXDE_TOP_SRCDIR "${CMAKE_SOURCE_DIR}")

set(GXDE_COMMON_DEFINES
    "PLUGINDIR=\"${GXDE_TOP_SRCDIR}/plugins:${GXDE_PLUGINDIR}\""
    "TOOLDIR=\"${GXDE_TOP_SRCDIR}/tools:${GXDE_TOOLDIR}\""
)

set(GXDE_COMMON_INCLUDES
    "${GXDE_TOP_SRCDIR}"
    "${GXDE_TOP_SRCDIR}/utils"
    "${GXDE_TOP_SRCDIR}/gxde-file-manager-lib/interfaces"
    "${GXDE_TOP_SRCDIR}/gxde-file-manager-lib/interfaces/plugins"
    "${GXDE_TOP_SRCDIR}/gxde-file-manager-plugins/plugininterfaces"
)

# 架构相关宏（对应 common.pri 中的 ARCH 判定）
set(GXDE_ARCH "${CMAKE_HOST_SYSTEM_PROCESSOR}")
set(GXDE_ARCH_DEFINES "")
if(GXDE_ARCH MATCHES "^(mips64|mips32|mips)$")
    list(APPEND GXDE_ARCH_DEFINES MENU_DIALOG_PLUGIN SPLICE_CP)
endif()

if(GXDE_ARCH MATCHES "^(sw_64|mips64|mips32|mips)$")
    list(APPEND GXDE_ARCH_DEFINES
        ARCH_MIPSEL ARCH_SW CLASSICAL_SECTION AUTO_RESTART_DEAMON
        LOAD_FILE_INTERVAL=150 DISABLE_COMPRESS_PREIVEW
        DISABLE_QUIT_ON_LAST_WINDOW_CLOSED)
    set(GXDE_USE_JEMALLOC OFF)
else()
    if(NOT DEFINED DISABLE_JEMALLOC OR NOT DISABLE_JEMALLOC)
        set(GXDE_USE_JEMALLOC ON)
    else()
        set(GXDE_USE_JEMALLOC OFF)
    endif()
endif()

if(GXDE_ARCH STREQUAL "sw_64")
    list(APPEND GXDE_ARCH_DEFINES SW_CPUINFO)
endif()

# 对应 common.pri 里 CONFIG += DISABLE_ANYTHING
if(NOT DEFINED GXDE_DISABLE_ANYTHING)
    set(GXDE_DISABLE_ANYTHING ON)
endif()
if(GXDE_DISABLE_ANYTHING)
    list(APPEND GXDE_ARCH_DEFINES DISABLE_QUICK_SEARCH DISABLE_TAG_SUPPORT)
endif()

# Qt 版本探测：优先 Qt6
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
set(GXDE_QT_VERSION_MAJOR ${QT_VERSION_MAJOR})

# 翻译生成（对应各 .pro 里 generate_translations.sh）
# 用法：gxde_compile_translations(<target> <ts_glob_dir>)
function(gxde_compile_translations target ts_dir)
    file(GLOB _ts_files "${ts_dir}/*.ts")
    if(NOT _ts_files)
        return()
    endif()
    # 优先使用 lrelease 工具
    find_program(LRELEASE_EXECUTABLE
        NAMES lrelease-qt6 lrelease6 lrelease
        HINTS
            /usr/lib/qt6/bin
            /usr/lib/qt6/libexec
            /usr/lib/x86_64-linux-gnu/qt6/bin
            /usr/lib/x86_64-linux-gnu/qt6/libexec
    )
    if(NOT LRELEASE_EXECUTABLE)
        message(WARNING "lrelease not found, skipping translations for ${target}")
        return()
    endif()
    set(_qm_files "")
    foreach(_ts ${_ts_files})
        get_filename_component(_base "${_ts}" NAME_WE)
        set(_qm "${ts_dir}/${_base}.qm")
        add_custom_command(
            OUTPUT "${_qm}"
            COMMAND "${LRELEASE_EXECUTABLE}" "${_ts}" -qm "${_qm}"
            DEPENDS "${_ts}"
            VERBATIM
        )
        list(APPEND _qm_files "${_qm}")
    endforeach()
    add_custom_target(${target}_qm ALL DEPENDS ${_qm_files})
    add_dependencies(${target} ${target}_qm)
endfunction()
