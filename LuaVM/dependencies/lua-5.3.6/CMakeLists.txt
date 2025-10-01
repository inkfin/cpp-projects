cmake_minimum_required(VERSION 3.16)
project(lua53 LANGUAGES C VERSION 5.3.6)

# Options
option(LUA_BUILD_SHARED "Build liblua as a shared library" OFF)
option(LUA_BUILD_TOOLS  "Build CLI tools: lua, luac" ON)
option(LUA_ENABLE_COMPAT_52 "Define LUA_COMPAT_5_2" ON)
option(LUA_ENABLE_READLINE  "Link readline for lua interpreter (UNIX-like)" OFF)

# C standard similar to Makefile's -std=gnu99
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# -------------------------
# Source lists mirroring Makefile (CORE_O / LIB_O / LUA_O / LUAC_O)
# -------------------------
set(LUA_CORE_SRCS
  lapi.c lcode.c lctype.c ldebug.c ldo.c ldump.c lfunc.c lgc.c llex.c
  lmem.c lobject.c lopcodes.c lparser.c lstate.c lstring.c ltable.c
  ltm.c lundump.c lvm.c lzio.c
)

set(LUA_LIB_SRCS
  lauxlib.c lbaselib.c lbitlib.c lcorolib.c ldblib.c liolib.c
  lmathlib.c loslib.c lstrlib.c ltablib.c lutf8lib.c loadlib.c linit.c
)

set(LUA_PUBLIC_HEADERS
  lua.h luaconf.h lualib.h lauxlib.h lua.hpp
)

# --- prepend "src/" for all paths ---
list(TRANSFORM LUA_CORE_SRCS        PREPEND "src/")
list(TRANSFORM LUA_LIB_SRCS         PREPEND "src/")
list(TRANSFORM LUA_PUBLIC_HEADERS   PREPEND "src/")

# Paths are relative to src/, so set the working directory.
get_filename_component(_HERE "${CMAKE_CURRENT_LIST_DIR}/src" ABSOLUTE)

# -------------------------
# Library target: liblua (static by default)
# -------------------------
if(LUA_BUILD_SHARED)
  add_library(lua SHARED ${LUA_CORE_SRCS} ${LUA_LIB_SRCS})
else()
  add_library(lua STATIC ${LUA_CORE_SRCS} ${LUA_LIB_SRCS})
endif()
add_library(lua::lua ALIAS lua)

# Include directories for build and install time
target_include_directories(lua PUBLIC
  $<BUILD_INTERFACE:${_HERE}>
  $<INSTALL_INTERFACE:include>
)

# Public headers for install()
set_target_properties(lua PROPERTIES
  OUTPUT_NAME lua
  POSITION_INDEPENDENT_CODE ON
  PUBLIC_HEADER "${LUA_PUBLIC_HEADERS}"
)

# Common warning level (roughly similar to -Wall -Wextra)
if (MSVC)
  target_compile_options(lua PRIVATE /W3)
else()
  target_compile_options(lua PRIVATE -Wall -Wextra)
endif()

# Optional compatibility macro
if(LUA_ENABLE_COMPAT_52)
  target_compile_definitions(lua PRIVATE LUA_COMPAT_5_2)
endif()

# -------------------------
# Platform feature macros / system libs (mirrors Makefile convenience targets)
# -------------------------
if(APPLE)
  # macosx: -DLUA_USE_MACOSX and (optionally) -lreadline for CLI
  target_compile_definitions(lua PRIVATE LUA_USE_MACOSX)
elseif(UNIX AND NOT APPLE)
  # linux/posix/solaris/bsd variants generally use POSIX/dlopen flags
  # The original Makefile sets -DLUA_USE_LINUX on the "linux" target,
  # and -DLUA_USE_POSIX/-DLUA_USE_DLOPEN elsewhere.
  # Choose the Linux macro by default on Linux, otherwise POSIX+dlopen.
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_compile_definitions(lua PRIVATE LUA_USE_LINUX)
  else()
    target_compile_definitions(lua PRIVATE LUA_USE_POSIX LUA_USE_DLOPEN)
  endif()
elseif(WIN32)
  # mingw uses DLL build flags in Makefile; for MSVC we don't need special libs.
  if(LUA_BUILD_SHARED)
    target_compile_definitions(lua PRIVATE LUA_BUILD_AS_DLL)
  endif()
endif()

# Link libm and libdl where appropriate (UNIX, not Apple for dl)
if(UNIX AND NOT APPLE)
  target_link_libraries(lua PUBLIC m)
  find_library(DL_LIB dl)
  if(DL_LIB)
    target_link_libraries(lua PUBLIC ${DL_LIB})
  endif()
endif()

# Suppress library warnings
if(MSVC)
  target_compile_options(lua PRIVATE /W0)
else()
  target_compile_options(lua PRIVATE -w)
endif()

# -------------------------
# CLI tools (lua, luac) — mirror how the Makefile links them
# -------------------------
if(LUA_BUILD_TOOLS)
  add_executable(lua_cli src/lua.c)
  target_link_libraries(lua_cli PRIVATE lua::lua)
  set_target_properties(lua_cli PROPERTIES OUTPUT_NAME lua)

  # Note: In your Makefile, LUAC_O = luac.o only
  add_executable(luac src/luac.c)
  target_link_libraries(luac PRIVATE lua::lua)

  # readline support (UNIX-like) for lua interpreter only, if requested
  if(LUA_ENABLE_READLINE AND (UNIX OR APPLE))
    find_path(READLINE_INCLUDE_DIR readline/readline.h)
    find_library(READLINE_LIBRARY readline)
    if(READLINE_INCLUDE_DIR AND READLINE_LIBRARY)
      target_compile_definitions(lua_cli PRIVATE LUA_USE_READLINE)
      target_include_directories(lua_cli PRIVATE ${READLINE_INCLUDE_DIR})
      target_link_libraries(lua_cli PRIVATE ${READLINE_LIBRARY})
      # Some platforms may also need -lncurses
      find_library(NCURSES_LIBRARY ncurses)
      if(NCURSES_LIBRARY)
        target_link_libraries(lua_cli PRIVATE ${NCURSES_LIBRARY})
      endif()
    else()
      message(WARNING "readline not found; LUA_ENABLE_READLINE ignored")
    endif()
  endif()
endif()

# -------------------------
# Install rules (library + public headers + optional tools)
# -------------------------
include(GNUInstallDirs)
install(TARGETS lua
  EXPORT luaTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

if(LUA_BUILD_TOOLS)
  install(TARGETS lua_cli luac
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
endif()

# Export a CMake package for find_package(lua CONFIG)
install(EXPORT luaTargets
  NAMESPACE lua::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lua
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/luaConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/luaConfig.cmake"
"include(\"\${CMAKE_CURRENT_LIST_DIR}/luaTargets.cmake\")\n"
)
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/luaConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/luaConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lua
)
