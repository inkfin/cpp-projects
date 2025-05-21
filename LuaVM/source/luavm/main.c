#include <stdio.h>
#include <stdlib.h>
#include <lualib.h>
#include <lauxlib.h>

// return 1 if success, 0 if failed
int CheckLua(lua_State* L, int res);

int main(int argc, char* argv[]) {
    lua_State* L = luaL_newstate();  // Create a new Lua state
    luaL_openlibs(L);  // Load the standard libraries

    if (CheckLua(L, luaL_dofile(L, "test.lua"))) {
        // Successfully executed the command
        lua_getglobal(L, "a");  // Get the value of 'a' from the Lua state
        if (CheckLua(L, lua_isnumber(L, -1))) {
            float a = (float)lua_tonumber(L, -1);
            fprintf(stdout, "a = %f\n", a);  // Print the value of 'a'
        }
    }

    system("pause");  // Pause the system (Windows specific)
    lua_close(L);
    return 0;
}

int CheckLua(lua_State* L, int res) {
    if (res != LUA_OK) {
        const char* errMsg = lua_tostring(L, -1);  // Get the error message
        fprintf(stderr, "Error: %s\n", errMsg);  // Print the error message
        lua_pop(L, 1);  // Remove the error message from the stack
        return 0;
    }
    return 1;
}
