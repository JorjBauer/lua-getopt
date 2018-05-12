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
local alpha = "unset"
local longopts = { alpha = { has_arg = "no_argument",
			     flag = "alpha",
			     val = "a" },
		}
local ret = getopt.long("a", longopts)

print (string.format("%s %s", tostring(ret), tostring(alpha)))
]])
tf:close()

posix.chmod(fn, "755")

local tests = {
   -- simple boolean tests: short; long
   [' -a'] = "true 97",
   [' --alpha'] = "true 97",
   -- incorrect attributes
   [' -b'] = "false unset",
   [' --bravo'] = "false unset",
 }

for k,v in pairs(tests) do
   io.write ("Running getopt.long no-return test '" .. k .. "'... ")
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

