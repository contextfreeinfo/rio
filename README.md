# Ion -> Rio


## Overview

At present, this is a port of the Ion compiler from C to Ion, derived directly
from Per Vognsen's [Bitwise](https://github.com/pervognsen/bitwise) project.

I've moved it here to my old Rio project to give myself more permission to
evolve it a different direction.
And Ion bootstraps me a lot further than I'd gotten on my own.

Updates might be coming.


## Usage

1. Add the bin dir to your path for convenience.
2. Call `bion`, which is a wrapper around the ion compiler.
   It manages some environment for you and even builds the compiler if it
   doesn't yet exist.
3. After ion outputs a c file for your project, you need to build the c using
   your native compiler.
   Ideally, that gets automated at some point, too.

That should work if you have gcc on Linux or msvc 2017 on Windows.

Older versions of msvc should also work, but you'll have to explicitly bring up
an msvc command prompt or environment first.
You should also set up a manual environment on Windows if you want to target x86
instead of x64.


## Motivation

See also the [Ion motivation document](https://github.com/pervognsen/bitwise/blob/master/notes/ion_motivation.md)
for some of the original goals of Ion.
In my own opinion, the non-goals of Ion (such the "no 'best language ever'
syndrome") have actually allowed it to be better than some alternatives.
It's unbloated.
It's pragmatic and serves very well as a better C.
It compiles fast and without effort.

This makes it a great core language and potentially a great language to tinker
on.

Just my own two cents.
