cmake_minimum_required(VERSION 3.29)

#===== Project Build Options ===========

project(LuaProjects
    LANGUAGES C CXX
    VERSION 0.1)

add_subdirectory(dependencies/lua)

add_subdirectory(source/luavm)
