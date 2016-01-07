#include <lua.h>
#include <lauxlib.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

#include "argv.h"
#include "options.h"
#include "set-lua-variable.h"

#define MODULENAME      "getopt"

#ifndef VERSION
#define VERSION "undefined"
#endif

#define ERROR(x) { lua_pushstring(l, x); lua_error(l); }
#define getn(L,n) (luaL_checktype(L, n, LUA_TTABLE), luaL_getn(L, n))

#if LUA_VERSION_NUM == 501
// Lua 5.1 has direct access to the lua globals; add a macro that makes 5.1 
// work the same as 5.2 and above.
#define lua_pushglobaltable(x) lua_pushvalue(x, LUA_GLOBALSINDEX)
#endif

/* version = getopt.version()                                            
 */
static int version(lua_State *l)
{
  int numargs = lua_gettop(l);
  if (numargs != 0) {
    ERROR("usage: getopt.version()");
  }

  lua_pushstring(l, VERSION);

  return 1;
}


/* bool result = getopt.std("opts", table)
 *
 * Uses the libc getopt() call and stuffs results in the given table.
 */

static int lgetopt_std(lua_State *l)
{
  const char *optstring = NULL;
  int result = 1; /* assume success */
  int argc, ch;
  char **argv = NULL;

  int numargs = lua_gettop(l);
  if (numargs != 2 ||
      lua_type(l,1) != LUA_TSTRING ||
      lua_type(l,2) != LUA_TTABLE) {
    ERROR("usage: getopt.std(optionstring, resulttable)");
  }

  optstring = lua_tostring(l, 1);

  /* Construct fake argc/argv from the magic lua 'arg' table. */
  lua_pushglobaltable(l);
  lua_getfield(l, -1, "arg");
  construct_args(l, lua_gettop(l), &argc, &argv);
  lua_pop(l, 1);

  /* Parse the options and store them in the Lua table. */
  while ((ch=getopt(argc, argv, optstring)) != -1) {
    char buf[2] = { ch, 0 };

    if (ch == '?') {
      /* This is the special "got a bad option" character. Don't put it 
       * in the table of results; just record that there's a failure.
       * The getopt call will have emitted an error to stderr. */
      result = 0;
      continue;
    }
    
    if (optarg) {
      lua_pushstring(l, optarg);
    } else {
      lua_pushboolean(l, 1);
    }
    lua_setfield(l, 2, buf);
  }

  free_args(argc, argv);

  /* Return 1 item on the stack (boolean) */
  lua_pushboolean(l, result);

  return 1; /* # of arguments returned on stack */
}

static void _call_callback(lua_State *l, int table_idx, 
			   struct option *longopts)
{
  /* Run through the list of options; first that matches
   * longopts->name, see if it's got a callback; if it does, call
   * it. */
  lua_pushnil(l);
  while (lua_next(l, table_idx) != 0) {
    // -2 is <key>; -1 is <val>
    if (lua_istable(l, -1)) {
      int this_table = lua_gettop(l);
      if (lua_isstring(l, -2)) {
	const char *sval = lua_tostring(l, -2);
	if (longopts->name && !strcmp(sval, longopts->name)) {
	  lua_pushstring(l, "callback");
	  lua_gettable(l, this_table);
	  if (lua_isfunction(l, -1)) {
	    lua_pushinteger(l, optind);
	    lua_call(l, 1, 1); // 1 argument, 1 result. Not protecting against errors.
	    
	    /* FIXME: if it returns something non-nil, should we stop processing? */
	  }
	  lua_pop(l, 3);
	  return;
	}
      }
    }
    lua_pop(l, 1);
  }
}

static int loptind(lua_State *l)
{
  lua_pushnumber(l, optind);
  return 1;
}

static int lsoptind(lua_State *l)
{
  optind = lua_tointeger(l, 1);
  return 0;
}

static int loptopt(lua_State *l)
{
  lua_pushnumber(l, optopt);
  return 1;
}

static int lopterr(lua_State *l)
{
  lua_pushnumber(l, opterr);
  return 1;
}

#if 0
static int loptreset(lua_State *l)
{
  lua_pushnumber(l, optreset);
  return 1;
}

static int lsoptreset(lua_State *l)
{
  optreset = lua_tointeger(l, 1);
  return 0;
}
#endif

static int loptarg(lua_State *l)
{
  if (optarg) {
    lua_pushstring(l, optarg);
  } else {
    lua_pushnil(l);
  }
  return 1;
}

/* bool result = getopt.long("opts", longopts_in, opts_out, error_function)
 *
 * Uses the libc getopt_long() call and stuffs results in the given table.
 */

typedef int (*func_t)(int argc, char * const *argv, const char *optstring,
		      const struct option *longopts, int *longindex);

static int lgetopt_long_t(lua_State *l, func_t func)
{
  const char *optstring = NULL;
  int result = 1; /* assume success */
  int argc, ch, idx;
  char **argv = NULL;
  int error_func = 0;

  int numargs = lua_gettop(l);
  if ((numargs != 3 && numargs != 4) ||
      lua_type(l,1) != LUA_TSTRING ||
      lua_type(l,2) != LUA_TTABLE ||
      lua_type(l,3) != LUA_TTABLE) {
    ERROR("usage: getopt.long(optionstring, longopts, resulttable, [errorfunc])");
  }
  if (numargs == 4 &&
      lua_type(l,4) != LUA_TFUNCTION) {
    ERROR("usage: getopt.long(optionstring, longopts, resulttable, [errorfunc])");
  }
  if (numargs == 4) {
    // We can't copy the error function - but we can make a
    // registry pointer.
    error_func = luaL_ref(l, LUA_REGISTRYINDEX);
  }

  optstring = lua_tostring(l, 1);

  /* Construct fake argc/argv from the magic lua 'arg' table. */
  lua_pushglobaltable(l);
  lua_getfield(l, -1, "arg");
  construct_args(l, lua_gettop(l), &argc, &argv);
  lua_pop(l, 1);

  /* Construct a longopts struct from the one given. */
  char **bound_variable_name = NULL;
  int *bound_variable_value = NULL;
  struct option *longopts = build_longopts(l, 2, 
					   &bound_variable_name,
					   &bound_variable_value);

  /* Parse the options and store them in the Lua table. */
  while ((ch=func(argc, argv, optstring, longopts, &idx)) != -1) {
    char buf[2] = { ch, 0 };

    printf("ch is %c; idx %d\n", ch, idx);

    if (ch == '?' || ch == ':') {
      /* This is a special "got a bad option" character. Don't put it 
       * in the table of results; just record that there's a failure.
       * The getopt call will have emitted an error to stderr. */

      if (numargs == 4) {
	lua_rawgeti(l, LUA_REGISTRYINDEX, error_func);
	lua_pushinteger(l, ch);
	lua_call(l, 1, 0); // 1 argument, 0 results. Not protecting against errors.
      }

      result = 0;
      printf("breaking\n");
      break;
    }

    /* Call any available callbacks for this element. */
    _call_callback(l, 2, &longopts[idx]);

    if (ch == 0) {
      /* This is the special "bound a variable" character. Don't put 
       * it in the table; look up the item at the right index and set it
       * appropriately. */
      set_lua_variable(l, bound_variable_name[idx], bound_variable_value[idx]);
      continue;
    }
    
    if (optarg) {
      lua_pushstring(l, optarg);
    } else {
      lua_pushboolean(l, 1);
    }
    lua_setfield(l, 3, buf);
  }

  free_longopts(longopts, bound_variable_name, bound_variable_value);
  free_args(argc, argv);

  /* Update args[] to remove the arguments we parsed */
  // decided I don't like this behavior. Instead, caller should get back optind
  // somehow and do it themselves.
  //  _remove_args(l, optind-1);

  if (numargs == 4) {
    luaL_unref(l, LUA_REGISTRYINDEX, error_func);
  }

  /* Return 1 item on the stack (boolean) */
  lua_pushboolean(l, result);

  return 1; /* # of arguments returned on stack */
}

static int lgetopt_long(lua_State *l)
{
  return lgetopt_long_t(l, getopt_long);
}

static int lgetopt_long_only(lua_State *l)
{
  return lgetopt_long_t(l, getopt_long_only);
}

/* metatable, hook for calling gc_context on context structs */
static const luaL_Reg meta[] = {
  /*  { "__gc", gc_context }, */
  { NULL,   NULL        }
};

/* function table for this module */
static const struct luaL_Reg methods[] = {
  { "version",      version           },
  { "std",          lgetopt_std       },
  { "long",         lgetopt_long      },
  { "long_only",    lgetopt_long_only },
  { "get_optind",   loptind           },
  { "set_optind",   lsoptind          },
  { "get_optopt",   loptopt           },
  { "get_opterr",   lopterr           },
#if 0
  { "get_optreset", loptreset         },
  { "set_optreset", lsoptreset        },
#endif
  { "get_optarg",   loptarg           },
  { NULL,           NULL              }
};

/* Module initializer, called from Lua when the module is loaded. */
int luaopen_getopt(lua_State *l)
{
  /* Construct a new namespace table for Lua, and register it as the global 
   * named "getopt".
   */
#if LUA_VERSION_NUM == 501
  luaL_openlib(l, MODULENAME, methods, 0);
#else
  lua_newtable(l);
  luaL_setfuncs(l, methods, 0);
#endif

  /* Create metatable, which is used to tie C data structures to our garbage 
   * collection function. */
  luaL_newmetatable(l, MODULENAME);

#if LUA_VERSION_NUM == 501
  luaL_openlib(l, 0, meta, 0);
#else
  luaL_setfuncs(l, meta, 0);
#endif

  lua_pushliteral(l, "__index");
  lua_pushvalue(l, -3);               /* dup methods table*/
  lua_rawset(l, -3);                  /* metatable.__index = methods */
  lua_pushliteral(l, "__metatable");
  lua_pushvalue(l, -3);               /* dup methods table*/
  lua_rawset(l, -3);                  /* hide metatable:
                                         metatable.__metatable = methods */
  lua_pop(l, 1);                      /* drop metatable */

  return 1;                           /* return methods on the stack */

}

