CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(SPACERACE)
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMakeModules/")

# Use folders
SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)
    

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wfatal-errors -Wall -Wno-unused-local-typedefs -ggdb -g  -D_GLIBCXX_USE_NANOSLEEP -fPIC -fopenmp -Wextra -pedantic -pthread")
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-std=c++11 -Wall -Wno-unused-local-typedefs -ggdb -g -D_GLIBCXX_USE_NANOSLEEP -DNDEBUG -O2 -fopenmp -pthread")
SET(CMAKE_CXX_FLAGS_RELEASE "-std=c++11 -D_GLIBCXX_USE_NANOSLEEP -DNDEBUG -O2 -fopenmp -pthread")

# Boost

SET(DESIRED_BOOST_VERSION "1_58_0")
SET(Boost_USE_STATIC_LIBS ON)
SET(Boost_USE_STATIC_RUNTIME ON)

FIND_PACKAGE(Boost REQUIRED COMPONENTS program_options
                                       system
                                       filesystem)

# ZeroMQ
FIND_PACKAGE(ZMQ REQUIRED)

ADD_SUBDIRECTORY(server)

#CONFIGURE_FILE(dummy_client.py "${SPACERACE_BINARY_DIR}/dummy_client.py" COPYONLY)
#CONFIGURE_FILE(server/spacerace.json "${SPACERACE_BINARY_DIR}/spacerace.json" COPYONLY)
