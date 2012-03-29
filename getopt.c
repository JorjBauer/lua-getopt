#include <lua.h>
#include <lauxlib.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "luaabstract.h"

#define MODULENAME      "getopt"

#ifndef VERSION
#define VERSION "undefined"
#endif

static void *_realloc_argv(char *argv[], int old_size, int new_size)
{
  char **new_argv = malloc(sizeof(char **) * new_size);

  if (argv) {
    memcpy(new_argv, argv, sizeof(char **) * old_size);
    free(argv);
  }

  return new_argv;
}

static void _add_arg(int *argv_size, int *argcp, char **argvp[], const char *new_arg)
{
  /* Make sure we have space for both the new element and the ending NULL 
   * terminating element. If not, make a newer, larger array. */
  if (*argcp+2 > *argv_size) {
    int new_argv_size = *argv_size * 2;
    *argvp = _realloc_argv(*argvp, *argv_size, new_argv_size);
    *argv_size = new_argv_size;
  }

  (*argvp)[*argcp] = malloc(strlen(new_arg)+1);
  strcpy((*argvp)[*argcp], new_arg);
  (*argcp)++;
}

static int _construct_args(lua_State *l, int idx, int *argcp, char ***argvp)
{
  int i=0;
  int argv_size = 10;
  *argvp = _realloc_argv(NULL, 0, argv_size);
  *argcp = 0;

  while (1) {
    /* Grab lua table element in index "idx" */
    lua_pushnumber(l, i);
    lua_gettable(l, idx);

    /* If the element on the top of the stack is nil, we're done. */
    if (lua_type(l, -1) == LUA_TNIL) {
      lua_pop(l, 1); /* Pop the NIL off the stack */
      break;
    } 

    /* Avoid calling lua_tolstring on a number; that would convert the 
     * actual element on the stack to a LUA_TSTRING, which apparently 
     * confuses Lua's iterators. */
    if (lua_type(l, -1) == LUA_TNUMBER) {
      const char *new_string;

      lua_pushfstring(l, "%f", lua_tonumber(l, -1));
      new_string = lua_tostring(l, -1);
      _add_arg(&argv_size, argcp, argvp, new_string);
      lua_pop(l, 1); /* Pop the fstring off the stack */
    }
    else if (lua_type(l, -1) == LUA_TSTRING) {
      const char *new_string = lua_tostring(l, -1);
      _add_arg(&argv_size, argcp, argvp, new_string);
    }
    else {
      /* Buh? Has someone been messing with arg[]? */
      _add_arg(&argv_size, argcp, argvp, "(null)");
    }
    lua_pop(l, 1); /* Pop the return value off the stack */
    
    i++;
  }
  (*argvp)[(*argcp)] = NULL;

  /* return a count of the number of entries we saw */
  return i;
}

static void _free_args(int argc, char *argv[])
{
  int i;
  for (i=0; i<argc; i++) {
    free(argv[i]);
  }
  free(argv);
}

/* bool result = getopt.std("opts", table)
 *
 * Uses the libc getopt() call and stuffs results in the given table.
 */

static int getopt_std(lua_State *l)
{
  const char *optstring = NULL;
  int result = 1; /* assume success */
  int argc, ch;
  char **argv = NULL;

  int numargs = lua_gettop(l);
  if (numargs != 2 ||
      lua_type(l,1) != LUA_TSTRING ||
      lua_type(l,2) != LUA_TTABLE) {
    lua_pushstring(l, "usage: getopt.std(optionstring, resulttable)");
    lua_error(l);
    return 0;
  }

  optstring = tostring(l, 1);

  /* Construct fake argc/argv from the magic lua 'arg' table. */
  lua_getfield(l, LUA_GLOBALSINDEX, "arg");
  _construct_args(l, lua_gettop(l), &argc, &argv);
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

  _free_args(argc, argv);

  /* Return 1 item on the stack (boolean) */
  lua_pushboolean(l, result);

  return 1; /* # of arguments returned on stack */
}


/* metatable, hook for calling gc_context on context structs */
static const luaL_reg meta[] = {
  /*  { "__gc", gc_context }, */
  { NULL,   NULL        }
};

/* function table for this module */
static const struct luaL_reg methods[] = {
  { "std",          getopt_std  },
  { NULL,           NULL        }
};

/* Module initializer, called from Lua when the module is loaded. */
int luaopen_getopt(lua_State *l)
{
  /* Construct a new namespace table for Lua, and register it as the global 
   * named "getopt".
   */
  luaL_openlib(l, MODULENAME, methods, 0);

  /* Create metatable, which is used to tie C data structures to our garbage 
   * collection function. */
  luaL_newmetatable(l, MODULENAME);
  luaL_openlib(l, 0, meta, 0);
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

