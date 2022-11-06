cmake_minimum_required(VERSION 3.10)

#set the project name
project(KEngineCore)

add_library(KEngineCore STATIC 
BinaryFile.h
BinaryFile.cpp
Disposable.h
Disposable.cpp
List.h
LuaLibrary.h
LuaLibrary.cpp
LuaScheduler.h
LuaScheduler.cpp
Pool.h
StringHash.h
StringHash.cpp
TextFile.h
TextFile.cpp
Timer.h
Timer.cpp
Updater.h
Updater.cpp
Lua.hpp
DataTree.h
DataTree.cpp
Psychopomp.h
Psychopomp.cpp
StringTable.h
StringTable.cpp
Tweening.h
Tweening.cpp
)

target_compile_features(KEngineCore PUBLIC cxx_std_20) 
target_include_directories(KEngineCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
add_subdirectory(boost-cmake)
set(BUILD_SHARED_LIBS OFF)
add_subdirectory(lua-cmake)
target_link_libraries(KEngineCore PUBLIC Boost::boost lua::header)
target_link_libraries(KEngineCore PRIVATE lua::lib)