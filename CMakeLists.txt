cmake_minimum_required(VERSION 3.10)

#set the project name
project(KEngineCore)

add_library(KEngineCore STATIC 
BinaryFile.h
BinaryFile.cpp
Disposable.h
Disposable.cpp
List.h
LuaContext.h
LuaContext.cpp
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
compile_time_crc.hpp
)

target_compile_features(KEngineCore PUBLIC cxx_std_20) 

# Clang doesn't support std::format so we pull in the fmt library
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
add_subdirectory(fmt)
endif()

target_include_directories(KEngineCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set(BUILD_SHARED_LIBS OFF)
add_subdirectory(lua-cmake)
target_link_libraries(KEngineCore PUBLIC lua::header fmt)
target_link_libraries(KEngineCore PRIVATE lua::lib)

option(KENGINE_CORE_DELETE_STRINGS_RELEASE "Delete strings from hashes in release builds" ON)

if (KENGINE_CORE_DELETE_STRINGS_RELEASE)
    target_compile_definitions(KEngineCore PRIVATE
        $<$<CONFIG:Release>:
            DELETE_STRINGS
        >
    )
endif()