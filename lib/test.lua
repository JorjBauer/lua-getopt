#!/usr/bin/env lua

local basedir = require 'findbin' '/..'
--print("# basedir: " .. basedir)
require 'lib' (basedir)
require 'lib' (basedir .. '/lib')
require 'DataDumper'

local getopt = require 'getopt'

--print("Testing getopt version " .. getopt.version())

local opts = {}

local ret = getopt.std("hb:a", opts)

if (ret) then
   print "return code: true"
else
   print "return code: false"
end
print (DataDumper(opts))

