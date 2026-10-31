# LuaLunaFive

C++ bindings for Lua from the Lua Users forum.

Code available from:

[http://lua-users.org/wiki/LuaLunaFive](http://lua-users.org/wiki/LuaLunaFive).

# Modifications

`luna.h` is a modified version of the original, with these changes:

- A class can have up to 65,535 methods and 65,535 properties.
  The original tagged methods with bit 8, so method 256 would silently call method 0.
- Method closures hold the object's userdata, which keeps the object alive while a closure refers to it.
- Scripts can't read metatable entries through an object (e.g., `obj.__gc` is `nil`), and `getmetatable(obj)` returns `false`.
- Calling a method or reading a property on a deleted object raises a Lua error.
- `pushBorrowed()` pushes an object that Lua doesn't own or delete.
  (`push()` still gives Lua ownership.)
- Setting a method raises its error through `luaL_error` directly, rather than using a built string as the format.

# Example

```cpp
LuaClass::LuaClass(lua_State* L)
{
	ref_property = LUA_REFNIL;
}

const char LuaClass::className[] = "luaclass";
const Luna<LuaClass>::PropertyType LuaClass::properties[] =
{
	{ "property", &LuaClass::lua_get_property, &LuaClass::lua_set_property },
	{ 0 }
};

int LuaClass::lua_set_property(lua_State* L)
{
	luaL_unref(L, LUA_REGISTRYINDEX, ref_format);
	ref_format = luaL_ref(L, LUA_REGISTRYINDEX);
	return 0;
}

int LuaClass::lua_get_property(lua_State* L)
{
	lua_rawgeti(L, LUA_REGISTRYINDEX, ref_format);
	return 1;
}

const Luna<LuaClass>::FunctionType LuaClass::methods[] =
{
	{ "func", &LuaClass::lua_func },
	{ 0 }
};

int LuaClass::lua_func(lua_State* L)
{
	const char* str = luaL_checkstring(L, -1);
	// do something
	lua_pushinteger(L, 1);
	return 1;
}
```
