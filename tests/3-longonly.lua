#!/usr/bin/env lua

--[[ 
   getopt.long_only() tests:
  
   Create a stub script and invoke it with various combinations of
   arguments. Inspect the output.
--]]

local posix = require 'posix'
local os = require "os"

local fn = os.tmpname()
local tf = assert(io.open(fn, "w+"))

tf:write([[#!/usr/bin/env lua
local getopt = require "getopt"
local opts = {}
local charlie = "unset"
local foxtrot = "unset"
local callbackcount = 0
local callbackarg
local longopts = { alpha = { has_arg = "no_argument",
			     val = "a" },
		   bravo = { has_arg = "required_argument",
			     val = "b" },
		   charlie = { has_arg = "required_argument",
			       flag = "charlie",
			       val = "c" },
		   delta = { has_arg = "no_argument",
			     callback = function(__unused) callbackcount = callbackcount + 1; end,
			     val = "d" },
		   echo = { has_arg = "required_argument",
			    callback = function(a) callbackarg = arg[a-1]; end,
			    val = "e" },
		   foxtrot = { has_arg = "no_argument",
			       flag = "foxtrot",
			       val = "f" },
		}
local ret = getopt.long_only("ab:c:de:f", longopts, opts, nil)

io.write(string.format("%s %s %s %s %s %d %s %s", tostring(ret), tostring(opts['a'] or "nil"), tostring(opts['b'] or "nil"), tostring(opts['c'] or "nil"), tostring(charlie or "nil"), callbackcount, tostring(callbackarg or "nil"), tostring(foxtrot or "nil")));
local p = getopt.get_optind()
if (p <= #arg) then
   io.write(" extras:")
   while (p <= #arg) do
      io.write(" " .. arg[p])
      p = p + 1
   end
end
io.write("\n")
]])
tf:close()

posix.chmod(fn, "755")

local tests = {
   -- simple boolean tests: short; long
   [' -a'] = "true true nil nil unset 0 nil unset",
   [' -alpha'] = "true true nil nil unset 0 nil unset",
   -- required argument, but it's missing
   [' -b'] = "false nil nil nil unset 0 nil unset",
   [' -bravo'] = "false nil nil nil unset 0 nil unset",
   -- required argument in four(-plus-one) flavors
   [' -b foo'] = "true nil foo nil unset 0 nil unset",
   [' -bfoo'] = "true nil foo nil unset 0 nil unset",
   [' -b=foo'] = "true nil =foo nil unset 0 nil unset",
   [' -bravo foo'] = "true nil foo nil unset 0 nil unset",
   [' -bravo=foo'] = "true nil foo nil unset 0 nil unset",
   -- required argument with variable bind; missing, plus flavors
   [' -charlie'] = "false nil nil nil unset 0 nil unset",
   [' -charlie foo'] = "true nil nil foo 99 0 nil unset",
   [' -charlie=foo'] = "true nil nil foo 99 0 nil unset",
   -- callback-without-argument tests
   [' -d'] = "true nil nil nil unset 1 nil unset",
   [' -delta'] = "true nil nil nil unset 1 nil unset",
   -- callback-with-argument tests
   [' -e'] = "false nil nil nil unset 0 nil unset",
   [' -echo'] = "false nil nil nil unset 0 nil unset",
   [' -e foo'] = "true nil nil nil unset 0 foo unset",
   [' -e=foo'] = "true nil nil nil unset 0 -e=foo unset",
   [' -echo foo'] = "true nil nil nil unset 0 foo unset",
   [' -echo=foo'] = "true nil nil nil unset 0 -echo=foo unset",
   -- multiple arguments
   [' -a -bravo=foo -charlie=bar -d -d -delta -echo baz'] = "true true foo bar 99 3 baz unset",
   -- multiple arguments but should be interrupted by a non-argument.
   -- The default behavior of getopt is to move the non-arguments to the end
   -- of argv, unless POSIXLY_CORRECT is set or the options string begins with
   -- a '+', at which point it bails at the first non-argument.
   [' -a notanarg -bravo=foo'] = "true true foo nil unset 0 nil unset extras: notanarg",
   -- test of simple longopt with a bound variable
   [' -f'] = "true nil nil nil unset 0 nil 102",
   [' -foxtrot'] = "true nil nil nil unset 0 nil 102",
 }

print "Running getopt.long_only tests..."
for k,v in pairs(tests) do
   io.write (" '" .. k .. "'... ")
   -- redirect stderr; we don't need to see the error output
   local fh = assert(io.popen(fn .. k .. " 2>/dev/null", 'r'))
   local output = fh:read("*l") -- read one line and compare...
   if (output == v) then
      print (" passed")
   else
      -- expected the value from the tests table, but got something else...
      print (" FAILED: got '" .. output .. "'")
   end
end

os.remove(fn)

