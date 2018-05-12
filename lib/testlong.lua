#!/usr/bin/env lua

local basedir = require 'findbin' '/..'
require 'lib' (basedir)
require 'lib' (basedir .. '/lib')

local getopt = require "getopt"
require 'lib.schwartzianTransforms'

--local posix = require "posix"
--posix.setenv("POSIXLY_CORRECT", 1)

-- print("Testing getopt version " .. getopt.version())


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
				-- consume the next argument manually via a callback
				callback = function(optind) print(">> I'm consuming the next argument manually: " .. tostring(arg[optind])); getopt.set_optind(getopt.get_optind()+1);  end
				},
		   reqargcb = { has_arg = "required_argument",
				val = "r",
				callback = function(optind) print(">>> callback w/ arg " .. arg[optind-1]); end
				},
		}

local retopts = {}

local daggerset = 0

local ret = getopt.long("a::cbf:r:", longopts, retopts,
			function(ch) print(">> error?: " .. ch); end)

if (ret) then
   print "return code: true"
else
   print "return code: false"
end
io.write("options: ")
print(table.concat(table.map(table.sort(table.keys(retopts)),
                             function(t,k,r) table.insert(r, tostring(k) .. "=" .. tostring(retopts[k])); end),
                   ";")
   )

print("daggerset==" .. daggerset)
print("optind==" .. getopt.get_optind())

local o = getopt.get_optind()
while (arg[o]) do
   print ("Unhandled argument: " .. arg[o])
   o = o + 1
end
