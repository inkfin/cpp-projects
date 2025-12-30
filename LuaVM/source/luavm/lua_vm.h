#ifndef LUA_VM_H
#define LUA_VM_H

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct LuaVM {
    lua_State* L;
} LuaVM;

/* lifecycle */
LuaVM* LuaVM_New(void);
void   LuaVM_Delete(LuaVM* vm);

/* run script */
int LuaVM_DoFile(LuaVM* vm, const char* path);
int LuaVM_DoString(LuaVM* vm, const char* code);

/* call a lua global function */
int LuaVM_Call(LuaVM* vm, const char* func, int argc, lua_Number* argv, int retc, lua_Number* rets);

/* get globals */
int LuaVM_GetNumber(LuaVM* vm, const char* name, lua_Number* out);
int LuaVM_GetString(LuaVM* vm, const char* name, const char** out);
int LuaVM_GetBool(LuaVM* vm, const char* name, int* out);

/* top-of-stack error */
const char* LuaVM_LastError(LuaVM* vm);

/* --------------------------------------------------------------------------
   Register a C function into Lua global namespace.
   Equivalent to:  _G[name] = func
---------------------------------------------------------------------------- */
void LuaVM_RegisterFunction(LuaVM* vm, const char* name, lua_CFunction func);

/* --------------------------------------------------------------------------
   Table field getters.
   They perform:
     1. lua_getfield()
     2. type checking
     3. write output
     4. pop stack
   Always leave the stack unchanged.
   Return 1 on success, 0 on failure (missing key or wrong type).
---------------------------------------------------------------------------- */
int LuaVM_GetTableNumber(lua_State* L, int idx, const char* key, lua_Number* out);
int LuaVM_GetTableInt(lua_State* L, int idx, const char* key, int* out);
int LuaVM_GetTableBool(lua_State* L, int idx, const char* key, int* out);
int LuaVM_GetTableString(lua_State* L, int idx, const char* key, const char** out);

/* --------------------------------------------------------------------------
   Array getters (1-based indexing).
   Same behavior as table getters:
     - read element
     - type check
     - pop
     - return 1/0
---------------------------------------------------------------------------- */
int LuaVM_GetArrayNumber(lua_State* L, int idx, int i, lua_Number* out);

/* --------------------------------------------------------------------------
   Return the array-like length of a table.
   Equivalent to: #table
   Uses lua_rawlen() for performance.
---------------------------------------------------------------------------- */
int LuaVM_TableLength(lua_State* L, int idx);

/* --------------------------------------------------------------------------
   Userdata helpers.
   LuaVM_NewUserdata:
       Allocates a Lua userdata block of given size.
   LuaVM_SetMetatable:
       Set metatable by name (must be registered beforehand).
---------------------------------------------------------------------------- */
void* LuaVM_NewUserdata(lua_State* L, size_t size);
void  LuaVM_SetMetatable(lua_State* L, const char* metaName);

#ifdef __cplusplus
}
#endif

#endif /* LUA_VM_H */
