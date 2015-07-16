
ADD_EXECUTABLE (spacerace-server main.cpp)

SET_TARGET_PROPERTIES(spacerace-server PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${SPACERACE_BINARY_DIR})

TARGET_LINK_LIBRARIES(spacerace-server ${ZMQ_LIBRARY}
                                       ${Boost_LOG_LIBRARY_RELEASE}
                                       ${Boost_THREAD_LIBRARY_RELEASE}
                                       ${Boost_SYSTEM_LIBRARY_RELEASE}
                                       ${Boost_PROGRAM_OPTIONS_LIBRARY_RELEASE})
