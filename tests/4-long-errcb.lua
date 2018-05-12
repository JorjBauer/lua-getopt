#!/usr/bin/env lua

--[[ 
   getopt.long() tests:
  
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
local longopts = { alpha = { has_arg = "no_argument",
			     val = "a" },
		   bravo = { has_arg = "required_argument",
			     val = "b" },
		}
local ret = getopt.long("ab:", longopts, opts, function(ch) print ("error " .. ch); end)

print (string.format("%s %s %s", tostring(ret), tostring(opts['a'] or "nil"), tostring(opts['b'] or "nil")));
]])
tf:close()

posix.chmod(fn, "755")

local tests = {
   -- simple boolean tests: short; long
   [' -a'] = "true true nil",
   [' --alpha'] = "true true nil",
   -- required argument, but it's missing: error '?'
   [' -b'] = "error ?",
   [' --bravo'] = "error ?",
 }

for k,v in pairs(tests) do
   io.write ("Running getopt.long error callback test '" .. k .. "'... ")
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

