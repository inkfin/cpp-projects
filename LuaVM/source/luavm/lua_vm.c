#include "lua_vm.h"
#include <stdio.h>
#include <stdlib.h>
static int LuaVM_Traceback(lua_State* L) {
    const char* msg = lua_tostring(L, 1);
    if (msg)
        luaL_traceback(L, L, msg, 1);
    else
        lua_pushliteral(L, "(error object is not a string)");
    return 1;
}

LuaVM* LuaVM_New(void) {
    LuaVM* vm = (LuaVM*)malloc(sizeof(LuaVM));
    if (!vm) return NULL;

    vm->L = luaL_newstate();
    if (!vm->L) {
        free(vm);
        return NULL;
    }

    luaL_openlibs(vm->L);
    return vm;
}

void LuaVM_Delete(LuaVM* vm) {
    if (vm) {
        if (vm->L) lua_close(vm->L);
        free(vm);
    }
}

static int LuaVM_DoCall(lua_State* L, int nargs, int nresults) {
    int base = lua_gettop(L) - nargs;
    lua_pushcfunction(L, LuaVM_Traceback);
    lua_insert(L, base);
    int status = lua_pcall(L, nargs, nresults, base);
    lua_remove(L, base);
    return status;
}

int LuaVM_DoFile(LuaVM* vm, const char* path) {
    lua_State* L = vm->L;

    if (luaL_loadfile(L, path) != LUA_OK) goto error;

    if (LuaVM_DoCall(L, 0, LUA_MULTRET) != LUA_OK) goto error;

    return 1;

error:
    fprintf(stderr, "Lua Error: %s\n", lua_tostring(L, -1));
    lua_pop(L, 1);
    return 0;
}

int LuaVM_DoString(LuaVM* vm, const char* code) {
    lua_State* L = vm->L;

    if (luaL_loadstring(L, code) != LUA_OK) goto error;

    if (LuaVM_DoCall(L, 0, LUA_MULTRET) != LUA_OK) goto error;

    return 1;

error:
    fprintf(stderr, "Lua Error: %s\n", lua_tostring(L, -1));
    lua_pop(L, 1);
    return 0;
}

int LuaVM_Call(LuaVM* vm, const char* func, int argc, lua_Number* argv, int retc, lua_Number* rets) {
    lua_State* L = vm->L;

    lua_getglobal(L, func);
    if (!lua_isfunction(L, -1)) {
        fprintf(stderr, "Lua Error: '%s' is not a function\n", func);
        lua_pop(L, 1);
        return 0;
    }

    for (int i = 0; i < argc; ++i) lua_pushnumber(L, argv[i]);

    if (LuaVM_DoCall(L, argc, retc) != LUA_OK) {
        fprintf(stderr, "Lua Error: %s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
        return 0;
    }

    for (int i = 0; i < retc; ++i) {
        int idx = -(retc - i);
        if (!lua_isnumber(L, idx)) {
            fprintf(stderr, "Lua Error: return[%d] is not number\n", i);
            lua_pop(L, retc);
            return 0;
        }
        rets[i] = lua_tonumber(L, idx);
    }

    lua_pop(L, retc);
    return 1;
}

int LuaVM_GetNumber(LuaVM* vm, const char* name, lua_Number* out) {
    lua_State* L = vm->L;
    lua_getglobal(L, name);

    if (!lua_isnumber(L, -1)) {
        lua_pop(L, 1);
        return 0;
    }

    *out = lua_tonumber(L, -1);
    lua_pop(L, 1);
    return 1;
}

int LuaVM_GetString(LuaVM* vm, const char* name, const char** out) {
    lua_State* L = vm->L;
    lua_getglobal(L, name);

    if (!lua_isstring(L, -1)) {
        lua_pop(L, 1);
        return 0;
    }

    *out = lua_tostring(L, -1);
    lua_pop(L, 1);
    return 1;
}

int LuaVM_GetBool(LuaVM* vm, const char* name, int* out) {
    lua_State* L = vm->L;
    lua_getglobal(L, name);

    if (!lua_isboolean(L, -1)) {
        lua_pop(L, 1);
        return 0;
    }

    *out = lua_toboolean(L, -1);
    lua_pop(L, 1);
    return 1;
}

const char* LuaVM_LastError(LuaVM* vm) {
    lua_State* L = vm->L;
    if (!lua_isstring(L, -1)) return NULL;
    return lua_tostring(L, -1);
}

/* ============================================================================
   Register a global C function:  _G[name] = func
============================================================================ */
void LuaVM_RegisterFunction(LuaVM* vm, const char* name, lua_CFunction func) { lua_register(vm->L, name, func); }

/* ============================================================================
   Table field getters – return 1 on success, 0 on failure.
   Stack is always balanced.
============================================================================ */
int LuaVM_GetTableNumber(lua_State* L, int idx, const char* key, lua_Number* out) {
    int abs = lua_absindex(L, idx);
    lua_getfield(L, abs, key);

    if (!lua_isnumber(L, -1)) {
        lua_pop(L, 1);
        return 0;
    }
    *out = lua_tonumber(L, -1);
    lua_pop(L, 1);
    return 1;
}

int LuaVM_GetTableInt(lua_State* L, int idx, const char* key, int* out) {
    int abs = lua_absindex(L, idx);
    lua_getfield(L, abs, key);

    if (!lua_isnumber(L, -1)) {
        lua_pop(L, 1);
        return 0;
    }
    *out = (int)lua_tointeger(L, -1);
    lua_pop(L, 1);
    return 1;
}

int LuaVM_GetTableBool(lua_State* L, int idx, const char* key, int* out) {
    int abs = lua_absindex(L, idx);
    lua_getfield(L, abs, key);

    if (!lua_isboolean(L, -1)) {
        lua_pop(L, 1);
        return 0;
    }
    *out = lua_toboolean(L, -1);
    lua_pop(L, 1);
    return 1;
}

int LuaVM_GetTableString(lua_State* L, int idx, const char* key, const char** out) {
    int abs = lua_absindex(L, idx);
    lua_getfield(L, abs, key);

    if (!lua_isstring(L, -1)) {
        lua_pop(L, 1);
        return 0;
    }
    *out = lua_tostring(L, -1);
    lua_pop(L, 1);
    return 1;
}

/* ============================================================================
   Array getter (1-based index).
============================================================================ */
int LuaVM_GetArrayNumber(lua_State* L, int idx, int i, lua_Number* out) {
    int abs = lua_absindex(L, idx);
    lua_rawgeti(L, abs, i);

    if (!lua_isnumber(L, -1)) {
        lua_pop(L, 1);
        return 0;
    }
    *out = lua_tonumber(L, -1);
    lua_pop(L, 1);
    return 1;
}

/* ============================================================================
   Table length (#table)
============================================================================ */
int LuaVM_TableLength(lua_State* L, int idx) {
    int abs = lua_absindex(L, idx);
    return (int)lua_rawlen(L, abs);
}

/* ============================================================================
   Userdata support
============================================================================ */
void* LuaVM_NewUserdata(lua_State* L, size_t size) { return lua_newuserdata(L, size); }

void LuaVM_SetMetatable(lua_State* L, const char* metaName) { luaL_setmetatable(L, metaName); }
