#include <lua.h>
#include <lauxlib.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>


#define ERROR(x) { lua_pushstring(l, x); lua_error(l); }
#define getn(L,n) (luaL_checktype(L, n, LUA_TTABLE), luaL_getn(L, n))

int get_call_stack_size(lua_State *l)
{
  int level = 0;
  lua_Debug ar;
  
  while (1) {
    if (lua_getstack(l, level, &ar) == 0) return level;
    level++;
  }
  /* NOTREACHED */
}

void set_lua_variable(lua_State *l, char *name, int value)
{
  /* See if there's a local variable that matches the given name.
   * If we find it, then set the value. If not, we'll keep walking back
   * up the stack levels until we've exhausted all of the closures.
   * At that point, set a global instead. */

  lua_Debug ar;
  int stacksize = get_call_stack_size(l);
  int stacklevel,i;

  /* This C call is stacklevel 0; the function that called is, 1; and so on. */
  for (stacklevel=0; stacklevel<stacksize; stacklevel++) {
    const char *local_name;
    lua_getstack(l, stacklevel, &ar);
    lua_getinfo(l, "nSluf", &ar); /* get all the info there is. Could probably be whittled down. */
    i=1;
    while ( (local_name=lua_getlocal(l, &ar, i++)) ) {
      if (!strcmp(name, local_name)) {
	/* Found the local! Set it, and exit. */
	lua_pop(l, 1);              // pop the local's old value
	lua_pushinteger(l, value);  // push the new value
	lua_setlocal(l, &ar, i-1); // set the value (note: i was incremented)
	lua_pop(l, 2);
	return;
      }
      lua_pop(l, 1);
    }
  }  

  /* Didn't find a local with that name anywhere. Set it as a global. */
  lua_pushinteger(l, value);
  lua_setglobal(l, name);
  lua_pop(l, 3);
}
