#!/usr/bin/env lua

local getopt = require "getopt"
local tu = require "tableUtils"

print("Testing getopt version " .. getopt.version())

local longopts = { buffy = { has_arg = "no_argument",
			       val = "b" },
		   fluoride = { has_arg = "required_argument",
				  val = "f" },
		   daggerset = { has_arg = "no_argument",
				   flag = "daggerset",
				   val = "1" }
		}

local retopts = {}

local daggerset = 0

local ret = getopt.long("bf:", longopts, retopts)

if (ret) then
   print "return code: true"
else
   print "return code: false"
end
print (tu.dump(retopts))
print("daggerset: " .. daggerset)

