set(TARGET_NAME video)

add_executable(${TARGET_NAME} main.cpp)

set_target_properties(${TARGET_NAME} PROPERTIES
    OUTPUT_NAME "video"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/tools/thumbnail"
)

target_link_libraries(${TARGET_NAME} PRIVATE ffmpegthumbnailer)

# 与qmake common.pri中TOOLDIR等价
install(TARGETS ${TARGET_NAME}
    RUNTIME DESTINATION "${GXDE_LIB_BASE_DIR}/tools/thumbnail")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/video.json"
    DESTINATION "${GXDE_LIB_BASE_DIR}/tools/thumbnail")
