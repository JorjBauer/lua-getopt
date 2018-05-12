#!/usr/bin/env lua

local basedir = require 'findbin' '/..'
--print("# basedir: " .. basedir)
require 'lib' (basedir)
require 'lib' (basedir .. '/lib')
local tu = require 'tableUtils'

local getopt = require 'getopt'

--print("Testing getopt version " .. getopt.version())

local opts = {}

local ret = getopt.std("hb:a", opts)

if (ret) then
   print "return code: true"
else
   print "return code: false"
end
print (tu.dump(opts))

local p = getopt.get_optind()
if (p <= #arg) then
   print "Remaining unhandled args: "
   while (p <= #arg) do
      print ("  " .. arg[p])
      p = p + 1
   end
end