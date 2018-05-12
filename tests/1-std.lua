#!/usr/bin/env lua

--[[ 
   getopt.std() tests:
  
   Create a stub script and invoke it with various combinations of
   arguments. Inspect the output.
--]]

local posix = require 'posix'
local os = require "os"

local fn = os.tmpname()
local tf = assert(io.open(fn, "w+"))

tf:write([[#!/usr/bin/env lua
local getopt = require 'getopt'
require "posix".setenv("POSIXLY_CORRECT", "1")
local opts = {}
local ret = getopt.std("h::b:a", opts)
io.write( string.format("%s %s %s %s", tostring(ret), tostring(opts['a'] or "nil"), tostring(opts['b'] or "nil"), tostring(opts['h'] or "nil") ) )
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
 -- simple no-argument test
 [' '] = "true nil nil nil",
 -- invalid switch test
 [' -z'] = "false nil nil nil",
 -- simple single argument test
 [' -a'] = "true true nil nil",
 -- failed argument test ("-b" requires an argument)
 [' -b'] = "false nil nil nil",
 -- successful argment test
 [' -b foo'] = "true nil foo nil",
 -- single extraneous argument test
 [' foo'] = "true nil nil nil extras: foo",
 -- argument after extraneous arg test, where 'bar' stops the processing.
 -- This test fails on Debian 8 unless we set POSIXLY_CORRECT in the test;
 -- without that, it leaves "bar -a" as extras, but it also processes the "-a"
 -- setting opt['a'] to true. Not a failure of this library; it's a difference
 -- in the underlying library's behavior. To leave in the test, I've set 
 -- POSIXLY_CORRECT in the test itself.
 [' -b foo bar -a'] = "true nil foo nil extras: bar -a",
 -- repeated argument test
 [' -a -a'] = "true true nil nil",
 -- optional argument missing value test probably fails on POSIX-compliant platforms; it's explicitly forbidden by POSIX standards
 -- [' -a -b foo -h'] = "true true foo true",
 -- with runon optional argument
 [' -a -b foo -hbar'] = "true true foo bar",
 [' -a -b foo -h=bar'] = "true true foo =bar",
 -- with space-separated optional argument. Some versions of getopt
 -- don't support this; they only support opt args where they're
 -- run-on ("-hbar"). MacOS 10.12 likes this, Debian 8 does not. Hence
 -- commenting it out as it's not a reliable indicator of library
 -- functionality.
 -- [' -a -b foo -h bar'] = "true true foo bar",
 }

for k,v in pairs(tests) do
   io.write ("Running getopt.std test '" .. k .. "'... ")
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

--os.remove(fn)

