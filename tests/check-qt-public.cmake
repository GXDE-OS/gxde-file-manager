find_program(READELF readelf REQUIRED)
execute_process(COMMAND "${READELF}" --dyn-syms --wide --demangle "${LIBRARY}"
    RESULT_VARIABLE result OUTPUT_VARIABLE symbols ERROR_VARIABLE error)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Cannot inspect ${LIBRARY}: ${error}")
endif()
if(symbols MATCHES "UND[^\n]*(PRIVATE_API|qt_blurImage|qt_findAtNxFile|platformNativeInterface|QNativeInterface::)")
    message(FATAL_ERROR "Direct dependency on an unsupported Qt API: ${CMAKE_MATCH_0}")
endif()
