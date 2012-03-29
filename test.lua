#!/usr/bin/env lua

local getopt = require "getopt"
local tu = require "tableUtils"

local opts = {}

--print(tu.dump(arg))

local ret = getopt.std("hb:a", opts)

if (ret) then
   print "return code: true"
else
   print "return code: false"
end
print (tu.dump(opts))

