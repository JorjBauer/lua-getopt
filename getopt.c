#include <lua.h>
#include <lauxlib.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

#define MODULENAME      "getopt"

#ifndef VERSION
#define VERSION "undefined"
#endif

#define ERROR(x) { lua_pushstring(l, x); lua_error(l); }
#define getn(L,n) (luaL_checktype(L, n, LUA_TTABLE), luaL_getn(L, n))

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

static int _count_options(lua_State *l, int table_idx)
{
  int count = 0;

  lua_pushnil(l);
  while (lua_next(l, table_idx) != 0) {
    count++;
    lua_pop(l, 1);
  }

  return count;
}

static void _populate_option(lua_State *l, 
			     struct option *p, 
			     char **bound_variable_name,
			     int *bound_variable_value,
			     int table_idx)
{
  // set defaults
  p->has_arg = no_argument;
  p->flag = NULL;
  p->val = 0;

  lua_pushnil(l);
  while (lua_next(l, table_idx) != 0) {
    // key is -2; value is -1
    // Expected keys: "has_arg", "flag" (optional), "val" (one character or #)

    if (lua_isstring(l, -2)) {
      const char *new_string = lua_tostring(l, -2);
      if (strcmp(new_string, "has_arg") == 0) {
	if (lua_isstring(l, -1)) {
	  const char *val = lua_tostring(l, -1);
	  if (strcmp(val, "no_argument") == 0) {
	    p->has_arg = no_argument;
	  } else if (strcmp(val, "required_argument") == 0) {
	    p->has_arg = required_argument;
	  } else if (strcmp(val, "optional_argument") == 0) {
	    p->has_arg = optional_argument;
	  } else {
	    ERROR("error: has_arg must be {no_argument|required_argument|optional_argument}");
	  }
	} else {
	  ERROR("error: has_arg must point to a string");
	}
      } else if (strcmp(new_string, "flag") == 0) {
	if (lua_isstring(l, -1)) {
	  const char *val = lua_tostring(l, -1);
	  *bound_variable_name = malloc(strlen(val)+1);
	  strcpy(*bound_variable_name, val);
	  p->flag = bound_variable_value;
	} else {
	  ERROR("error: flag must point to a string");
	}
      } else if (strcmp(new_string, "val") == 0) {
	if (lua_isnumber(l, -1)) {
	  p->val = lua_tonumber(l, -1);
	} else {
	  const char *val = lua_tostring(l, -1);
	  if (val[0] >= '0'  && val[0] <= '9') {
	    p->val = lua_tonumber(l, -1);
	  } else {
	    p->val = val[0];
	  }
	}
      } else if (strcmp(new_string, "callback")) {
	ERROR("error: longopts must be {has_arg|flag|val|callback}");
      }

    } else {
      ERROR("error: inappropriate non-string key in longopts");
    }

    lua_pop(l, 1);
  }

}

static const char *_safe_string(lua_State *l, int idx)
{
  char *ret = NULL;

  if (lua_type(l, idx) == LUA_TNUMBER) {
    const char *new_string;
    lua_pushfstring(l, "%f", lua_tonumber(l, idx));
    new_string = lua_tostring(l, -1);
    ret = malloc(strlen(new_string)+1);
    strcpy(ret, new_string);
    lua_pop(l, 1); /* Pop the fstring off the stack */
  }
  else if (lua_type(l, idx) == LUA_TSTRING) {
    const char *new_string = lua_tostring(l, idx);
    ret = malloc(strlen(new_string)+1);
    strcpy(ret, new_string);
  }
  else {
    ERROR("error: inappropriate non-string, non-number key in longopts");
  }
  return ret;
}

static struct option * _build_longopts(lua_State *l,
				       int table_idx,
				       char **bound_variable_name[],
				       int *bound_variable_value[])
{
  // Figure out the number of elements
  int num_opts = _count_options(l, table_idx);

  // alloc bound_variable_name & value; initialize the former to NULLs
  *bound_variable_name = malloc(sizeof(char**) * num_opts);
  memset(*bound_variable_name, 0, sizeof(char **) * num_opts);
  *bound_variable_value = malloc(sizeof(int*) * num_opts);

  // alloc longopts, plus room for NULL terminator
  struct option *ret = malloc(sizeof(struct option) * num_opts+1);
  int i = 0;

  // loop over the elements; for each, create a longopts struct
  lua_pushnil(l);
  while (lua_next(l, table_idx) != 0) {
    struct option *p = &ret[i];

    // key is -2, value is -1; don't lua_tolstring() numbers!
    const char *keyname = _safe_string(l, -2);
    p->name = keyname;
    
    // The value (idx==-1) is a table. Use the values in that table to 
    // populate the rest of the struct
    _populate_option(l, p, &(*bound_variable_name)[i], 
		     &(*bound_variable_value)[i],
		     lua_gettop(l));
    lua_pop(l, 1); // pop value; leave key
    i++;
  }

  // add NULL terminator
  ret[num_opts].name = NULL;

  return ret;
}

static void _free_longopts(struct option *longopts, 
			   char *bound_variable_name[],
			   int bound_variable_value[])
{
  int i = 0;
  struct option *p = longopts;

  while (p && p->name) {
    free((char *)p->name); /* willingly discard 'const' */
    if (bound_variable_name[i]) {
      free(bound_variable_name[i]);
    }
    i++;
    p++;
  }

  free(longopts);
  free(bound_variable_name);
  free(bound_variable_value);
}

static int _get_call_stack_size(lua_State *l)
{
  int level = 0;
  lua_Debug ar;
  
  while (1) {
    if (lua_getstack(l, level, &ar) == 0) return level;
    level++;
  }
  /* NOTREACHED */
}

static void _set_lua_variable(lua_State *l, char *name, int value)
{
  /* See if there's a local variable that matches the given name.
   * If we find it, then set the value. If not, we'll keep walking back
   * up the stack levels until we've exhausted all of the closures.
   * At that point, set a global instead. */

  lua_Debug ar;
  int stacksize = _get_call_stack_size(l);
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
	lua_pushnumber(l, value);  // push the new value
	lua_setlocal(l, &ar, i-1); // set the value (note: i was incremented)
	lua_pop(l, 2);
	return;
      }
      lua_pop(l, 1);
    }
  }  

  /* Didn't find a local with that name anywhere. Set it as a global. */
  lua_pushnumber(l, value);
  lua_setglobal(l, name);
  lua_pop(l, 3);
}

/* Remove num args from args[], starting at [1] */
static void _remove_args(lua_State *l, int num)
{
  int i;

  /* Make sure there's something to do */
  if (num == 0)
    return;

  lua_getglobal(l, "arg"); // push global 'arg' table on to stack
  int tbl_idx = lua_gettop(l);
  int max_element = getn(l, -1);

  /* For old elements that are staying, move them down. */
  for (i=num+1; i<=max_element; i++) {
    lua_rawgeti(l, tbl_idx, i);
    lua_rawseti(l, tbl_idx, i-num);
  }
  /* For any elements that moved but weren't replaced, remove them. */
  for (i=max_element - num + 1; i<=max_element; i++) {
    lua_pushnil(l);
    lua_rawseti(l, tbl_idx, i);
  }
  lua_pop(l, 1);
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
      const char *sval = _safe_string(l, -2);
      if (!strcmp(sval, longopts->name)) {
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

#ifdef _BSD_SOURCE
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
 * Modifies the global arg[], removing any options that were processed by 
 * getopt.long(). (This enables the caller to process the remaining arguemnts
 * manually after a "--" argument.)
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
  lua_getfield(l, LUA_GLOBALSINDEX, "arg");
  _construct_args(l, lua_gettop(l), &argc, &argv);
  lua_pop(l, 1);

  /* Construct a longopts struct from the one given. */
  char **bound_variable_name = NULL;
  int *bound_variable_value = NULL;
  struct option *longopts = _build_longopts(l, 2, 
					    &bound_variable_name,
					    &bound_variable_value);

  /* Parse the options and store them in the Lua table. */
  while ((ch=func(argc, argv, optstring, longopts, &idx)) != -1) {
    char buf[2] = { ch, 0 };

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
      continue;
    }

    /* Call any available callbacks for this element. */
    _call_callback(l, 2, &longopts[idx]);

    if (ch == 0) {
      /* This is the special "bound a variable" character. Don't put 
       * it in the table; look up the item at the right index and set it
       * appropriately. */
      _set_lua_variable(l, bound_variable_name[idx], bound_variable_value[idx]);
      continue;
    }
    
    if (optarg) {
      lua_pushstring(l, optarg);
    } else {
      lua_pushboolean(l, 1);
    }
    lua_setfield(l, 3, buf);
  }

  _free_longopts(longopts, bound_variable_name, bound_variable_value);
  _free_args(argc, argv);

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
static const luaL_reg meta[] = {
  /*  { "__gc", gc_context }, */
  { NULL,   NULL        }
};

/* function table for this module */
static const struct luaL_reg methods[] = {
  { "version",      version           },
  { "std",          lgetopt_std       },
  { "long",         lgetopt_long      },
  { "long_only",    lgetopt_long_only },
  { "get_optind",   loptind           },
  { "set_optind",   lsoptind          },
  { "get_optopt",   loptopt           },
  { "get_opterr",   lopterr           },
#ifdef _BSD_SOURCE
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

