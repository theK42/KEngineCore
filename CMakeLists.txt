cmake_minimum_required(VERSION 3.10)

#set the project name
project(KEngineCore)

add_library(KEngineCore STATIC 
BinaryFile.h
BinaryFile.cpp
List.h
LuaLibrary.h
LuaLibrary.cpp
LuaScheduler.h
LuaScheduler.cpp
StringHash.h
StringHash.cpp
TextFile.h
TextFile.cpp
Timer.h
Timer.cpp
Updater.h
Updater.cpp
Lua.hpp
)
target_include_directories(KEngineCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
add_subdirectory(boost-cmake)
set(BUILD_SHARED_LIBS OFF)
add_subdirectory(lua-cmake)
target_link_libraries(KEngineCore PUBLIC Boost::boost lua::lib)