#!/usr/bin/env lua

local basedir = require 'findbin' '/..'
require 'lib' (basedir)
require 'lib' (basedir .. '/lib')
require 'DataDumper'

local getopt = require "getopt"
local tu = require "tableUtils"

--posix.setenv("POSIXLY_CORRECT", 1)

--print("Testing getopt version " .. getopt.version())


-- Set up the long options:
--    if "--buffy" is passed, set [b];
--    if "--fluoride" is passed, set [f];
--    if "--angel" and optional argument are passed, set [a];
--    if "--daggerset" and a value are set, set the variable named 'daggerset';
--    if "--callback" and a value are set, then call a callback function with the given value
--
-- .. Note that these don't exactly line up with the short options (below). In particular, 
-- there is no short option for -a, -c, or daggerset.

local longopts = { buffy = { has_arg = "no_argument",
			       val = "b" },
		   fluoride = { has_arg = "required_argument",
				  val = "f" },
		   angel = { has_arg = "optional_argument",
			     val = "a" },
		   -- daggerset option sets the variable named 'daggerset' to the value '1'
		   daggerset = { has_arg = "no_argument",
				   flag = "daggerset",
				   val = "1" },
		   callback = { has_arg = "no_argument",
				val = "c",
				-- consume the next argument manually
				callback = function(optind) print(">> callback with optind "..optind.. " which means the next arg is " .. arg[optind]); getopt.set_optind(getopt.get_optind()+1); end
				},
		}

local retopts = {}

local daggerset = 0

local ret = getopt.long("bf:", longopts, retopts,
			function(ch) print(">> error?: " .. ch); end)

if (ret) then
   print "return code: true"
else
   print "return code: false"
end
print ("options: " .. DataDumper(retopts))
print("daggerset==" .. daggerset)
print("optind==" .. getopt.get_optind())
