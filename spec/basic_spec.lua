#!/usr/bin/env busted

arg[0] = arg[1]

local basedir = require 'findbin' '/..'
require 'lib' (basedir)
require 'lib' (basedir .. '/lib')
local io = require 'io'
local posix = require 'posix'
local path = require 'path'
local cwd = path.currentdir()

local cmd = cwd .. "/lib/test.lua"

function popen3(path, ...)
   local r1, w1 = posix.pipe()
   local r2, w2 = posix.pipe()
   local r3, w3 = posix.pipe()

   assert((w1 ~= nil and r2 ~= nil and r3 ~= nil), "pipe() failed")

   local pid, err = posix.fork()
   assert(pid ~= nil, "fork() failed")
   if pid == 0 then
      posix.close(w1)
      posix.close(r2)
      posix.close(r3)

      posix.dup2(r1, posix.fileno(io.stdin))
      posix.dup2(w2, posix.fileno(io.stdout))
      posix.dup2(w3, posix.fileno(io.stderr))

      local ret, err = posix.execp(path, ...)
      assert(ret ~= nil, "execp() failed: " .. err)

      posix._exit(1)
      return
   end

   posix.close(r1)
   posix.close(w2)
   posix.close(w3)

   return pid, w1, r2, r3
end

function run_test (...)
   local pid, w1, r2, r3 = popen3(cmd, ...)

   -- Grab the output
   local exitpid, cause, status = posix.wait(pid)

   -- return stderr concat'd with stdout
   return posix.read(r3, 65535) .. posix.read(r2, 65535)
end

-- Run permutations of getopt (via lib/test.lua).
describe("test of short flags", 
	 function()
	    it("runs a -h test that should be successful", 
	       function()
		  assert.equals("return code: true\nreturn { h=true }\n",
				run_test("-h"))
	       end
	    )
	    it("runs a -b test that should fail",
	       function()
		  assert.equals(cmd..": option requires an argument -- b\nreturn code: false\nreturn {  }\n",
		     run_test("-b"))
	       end
	    )
	    it("runs a -b test that should succeed",
	       function()
		  assert.equals("return code: true\nreturn { b=\"foo\" }\n",
		     run_test("-b","foo"))
	       end
	    )

	 end
      )

-- expect correct version

-- test bad option ("-q")
-- test good option ("-h")
-- test bad use of option-plus-string ("-b")
-- test good use of option-plus-string ("-b foo")

