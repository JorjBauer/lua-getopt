getopt library for Lua 5.1+

(c) 2015-2018 Jorj Bauer <jorj@jorj.org>

This is released under a BSD license; see the file LICENSE for details.

# Installation instructions

You want to use luarocks. You can install this with a simple

```
$ luarocks make rockspec/getopt-1.0.1-1.rockspec
```

There is an included Makefile, which did work the last time I tested
it; it's messy, error prone, and requires that you edit it to suit
your environment. YMMV.

# Usage

Short flags are trivial and function per the POSIX standard:

``` lua
local getopt = require 'getopt'
local opts = {}
local ret = getopt.std("h::b:a", opts)
```

... at which point opts[] contains the flags that were passed in. In
this case, 'h' has an optional argument; 'b' has a required argument;
'a' has no argument.

Long flags require some additional setup. See the 'getopt_long' man
page for general details about how this structure functions; the Lua
structure mirrors the C structure.

``` lua
local getopt = require 'getopt'
local opts = {}
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
local ret = getopt.long("ab:c:de:f", longopts, opts, nil)
```

# Bugs

* More tests need to be written! Things like...

  o POSIXALLY_CORRECT environment variable (and leading '+') behavior
  o intentionally passing invalid types of options to getopt.std and .long
  o hacking optind during a callback
  o more thorough optional argument testing (and short '::')
  o error callback testing with returned ":" rather than "?"

* The busted-based tests (in the spec/ subdirectory) are fragile. They
  depend on output that's getopt-implementation-specific, and require
  modules that aren't part of luarocks. (This is why I built the new
  set of tests in the tests/ subdirectory without 'busted'.)

* Much of this code is repurposed from my lua-cyrussasl module. Which
  means it should be abstracted in some common way, rather than
  copy-and-pasted.