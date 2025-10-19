/*
 * lua_debug.h - Tinylib lua debug
 */
#ifndef TL_LUA_DEBUG_H
#define TL_LUA_DEBUG_H

#include <lua.h>
#include <lauxlib.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef TL_LUA_NO_ABBR
#define try_get_typename(L, idx) tll_try_get_typename(L, idx)
#define preview_table(L, idx, max_pairs) tll_preview_table(L, idx, max_pairs)
#define dump_value(L, idx) tll_dump_value(L, idx)
#define dump_stack(L, tag) tll_dump_stack(L, tag)
#endif

const char* tll_try_get_typename(lua_State* L, int idx);
void tll_preview_table(lua_State* L, int idx, int max_pairs);
void tll_dump_value(lua_State* L, int idx);
void tll_dump_stack(lua_State* L, const char* tag);

#ifdef __cplusplus
}
#endif

#ifdef TL_LUA_DEBUG_IMPLEMENTATION

#ifdef __cplusplus
extern "C" {
#endif

// 读取元表中 __name，拿不到就返回 NULL（不改变栈形状）
const char* tll_try_get_typename(lua_State* L, int idx) {
    int t = lua_type(L, idx);
    if (t != LUA_TUSERDATA && t != LUA_TTABLE) return NULL;
    int abs = lua_absindex(L, idx);
    if (!lua_getmetatable(L, abs)) return NULL;  // +1 (mt)
    lua_getfield(L, -1, "__name");               // +1 (name?)
    const char* name = lua_isstring(L, -1) ? lua_tostring(L, -1) : NULL;
    lua_pop(L, 2);  // pop name, mt
    return name;
}

// 预览 table 的若干项，不递归深入
void tll_preview_table(lua_State* L, int idx, int max_pairs) {
    int abs = lua_absindex(L, idx);
    int count = 0;
    // 栈: ... （我们只会 push 临时 key/value 并成对 pop 掉）
    lua_pushnil(L);                  // first key (nil)
    while (lua_next(L, abs) != 0) {  // stack: ... key value
        // key 在 -2，value 在 -1；我们用 luaL_tolstring 打印（会额外 push
        // 字符串）
        size_t klen = 0, vlen = 0;
        const char* kstr = luaL_tolstring(L, -2, &klen);  // ... key value kstr
        const char* vstr = luaL_tolstring(
            L, -2,
            &vlen);  // 注意：-2 现在是 value 了（前一次 tolstring 多 push 了）
        printf("      [%d] %.*s : %.*s\n", count + 1, (int)klen, kstr,
               (int)vlen, vstr);
        lua_pop(L, 2);  // 弹掉两次 tolstring 的结果
        lua_pop(L, 1);  // 弹掉 value，只留 key 供下一轮 lua_next
        if (++count >= max_pairs) {
            printf("      ... (truncated)\n");
            break;
        }
        lua_pushvalue(L, -1);  // 复制 key 以稳住下一轮（可选）
        lua_remove(L, -2);     // 去掉旧 key、保留复制（使顺序更直观）
    }
}

// 打印单个索引位置的值（不改变最终栈高）
void tll_dump_value(lua_State* L, int idx) {
    int t = lua_type(L, idx);
    switch (t) {
        case LUA_TNIL:
            printf("nil\n");
            break;
        case LUA_TBOOLEAN:
            printf("boolean: %s\n", lua_toboolean(L, idx) ? "true" : "false");
            break;
        case LUA_TNUMBER:
            if (lua_isinteger(L, idx))
                printf("integer: %lld\n", (long long)lua_tointeger(L, idx));
            else
                printf("number: %.*g\n", 17, (double)lua_tonumber(L, idx));
            break;
        case LUA_TSTRING: {
            size_t len = 0;
            const char* s = lua_tolstring(L, idx, &len);
            // 注意：lua_tolstring 不会 push 新值（与 luaL_tolstring 不同）
            printf("string(len=%zu): \"%.*s\"\n", len, (int)len, s);
            break;
        }
        case LUA_TTABLE: {
            lua_Integer arr = (lua_Integer)lua_rawlen(L, idx);
            const char* mt = tll_try_get_typename(L, idx);
            printf("table: array_len=%lld", (long long)arr);
            if (mt) printf(", mt.__name=\"%s\"", mt);
            printf("\n");
            tll_preview_table(L, idx, 5);  // 预览最多 5 对
            break;
        }
        case LUA_TFUNCTION:
            if (lua_iscfunction(L, idx))
                printf("cfunction: %p\n", lua_tocfunction(L, idx));
            else
                printf("function (Lua): %p\n", lua_topointer(L, idx));
            break;
        case LUA_TLIGHTUSERDATA:
            printf("lightuserdata: %p\n", lua_touserdata(L, idx));
            break;
        case LUA_TUSERDATA: {
            void* p = lua_touserdata(L, idx);
            const char* mt = tll_try_get_typename(L, idx);
            printf("userdata: %p", p);
            if (mt) printf(", mt.__name=\"%s\"", mt);
            printf("\n");
            break;
        }
        case LUA_TTHREAD:
            printf("thread: %p\n", lua_tothread(L, idx));
            break;
        default:
            printf("unknown type (%d)\n", t);
            break;
    }
}

// 打印整个栈（不改变最终栈形状）
void tll_dump_stack(lua_State* L, const char* tag) {
    int top = lua_gettop(L);
    printf("==== Lua stack dump (%s), top=%d ====\n", tag ? tag : "", top);
    for (int i = 1; i <= top; ++i) {
        int abs = lua_absindex(L, i);
        int tt = lua_type(L, abs);
        printf("[%2d/%2d] (%s) ", i, top, lua_typename(L, tt));
        tll_dump_value(L, abs);
    }
    printf("========================================\n");
}

#ifdef __cplusplus
}
#endif

#endif  // TL_LUA_DEBUG_IMPLEMENTATION

#endif  // TL_LUA_DEBUG_H
