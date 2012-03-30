#!/usr/bin/env lua

local getopt = require "getopt"
local tu = require "tableUtils"
--local posix = require "posix"

--posix.setenv("POSIXLY_CORRECT", 1)

print("Testing getopt version " .. getopt.version())

print("arg[]: " .. tu.dump(arg))

local longopts = { buffy = { has_arg = "no_argument",
			       val = "b" },
		   fluoride = { has_arg = "required_argument",
				  val = "f" },
		   angel = { has_arg = "optional_argument",
			     val = "a" },
		   daggerset = { has_arg = "no_argument",
				   flag = "daggerset",
				   val = "1" },
		   callback = { has_arg = "no_argument",
				val = "c",
				callback = function(optind) print(">> callback with optind "..optind); getopt.set_optind(getopt.get_optind()+1); end
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
print ("returned options: " .. tu.dump(retopts))
print("daggerset==" .. daggerset)
print("optind==" .. getopt.get_optind())
