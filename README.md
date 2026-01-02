# Rio programming language

To build, do this (where Lua's just a fast pragmatic test target):

- cd engine; temper watch -t lua

Then after the first round finishes, in a different tab, do this:

- cd rio; cargo build --release # or --profile release-lto

There should be actual tests to run for the `rio` cli app sometime also.
