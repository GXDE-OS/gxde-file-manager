cmake_minimum_required(VERSION 3.16)
project(gxde_public_qt_tests LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
find_package(Qt6 REQUIRED COMPONENTS Widgets Test)
enable_testing()
find_package(GxdeBerneseQt6 0.1 CONFIG REQUIRED)

# Optional integration test against the actual freshly built file-manager library.
set(GXDE_LIBRARY_DIR "" CACHE PATH "Directory containing libgxde-file-manager.so")
if(GXDE_LIBRARY_DIR)
    find_package(Qt6 REQUIRED COMPONENTS Core5Compat)
    find_library(GXDE_FILE_MANAGER_LIBRARY NAMES gxde-file-manager
        PATHS "${GXDE_LIBRARY_DIR}" NO_DEFAULT_PATH REQUIRED)
    foreach(number RANGE 1 2)
        add_library(factory-fixture-${number} MODULE factory-fixture.cpp)
        target_link_libraries(factory-fixture-${number} PRIVATE Qt6::Core)
        set_target_properties(factory-fixture-${number} PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/fixtures")
    endforeach()
    add_executable(factoryloader-smoke factoryloader-smoke.cpp)
    target_include_directories(factoryloader-smoke PRIVATE
        ../gxde-file-manager-lib/interfaces ../gxde-file-manager-lib/interfaces/plugins)
    target_link_libraries(factoryloader-smoke PRIVATE
        Qt6::Widgets Qt6::Test Qt6::Core5Compat GxdeBernese::Qt6 "${GXDE_FILE_MANAGER_LIBRARY}")
    add_dependencies(factoryloader-smoke factory-fixture-1 factory-fixture-2)
    add_test(NAME factoryloader-smoke COMMAND factoryloader-smoke)
    add_test(NAME qt-public-symbols COMMAND ${CMAKE_COMMAND}
        -DLIBRARY=${GXDE_FILE_MANAGER_LIBRARY}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check-qt-public.cmake)
    set_tests_properties(factoryloader-smoke PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;DFM_PLUGIN_PATH=${CMAKE_CURRENT_BINARY_DIR}/fixtures")
endif()
