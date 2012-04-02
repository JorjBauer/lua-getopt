#include <lua.h>
#include <lauxlib.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

#include "argv.h"

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

int construct_args(lua_State *l, int idx, int *argcp, char ***argvp)
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

void free_args(int argc, char *argv[])
{
  int i;
  for (i=0; i<argc; i++) {
    free(argv[i]);
  }
  free(argv);
}

