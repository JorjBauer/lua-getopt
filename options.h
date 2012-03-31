struct option * _build_longopts(lua_State *l,
				int table_idx,
				char **bound_variable_name[],
				int *bound_variable_value[]);

void _free_longopts(struct option *longopts, 
		    char *bound_variable_name[],
		    int bound_variable_value[]);
