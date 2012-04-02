#include <lua.h>
#include <lauxlib.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

#include "argv.h"

#define ERROR(x) { lua_pushstring(l, x); lua_error(l); }
#define getn(L,n) (luaL_checktype(L, n, LUA_TTABLE), luaL_getn(L, n))

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

struct option * build_longopts(lua_State *l,
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

void free_longopts(struct option *longopts, 
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
