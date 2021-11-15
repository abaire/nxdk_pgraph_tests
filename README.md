nxdk_pgraph_tests
====

Various tests for nv2a rendering.

Based on code from https://github.com/Ernegien/nxdk/tree/test/texture-formats/samples/texture-formats

## Running with CLion

Create a build target

1. Create a new `Embedded GDB Server` target
1. Set the Target to `all`
1. Set the Executable to `main.exe`
1. Set `Download executable` to `None`
1. Set `'target remote' args` to `127.0.0.1:1234`
1. Set `GDB Server` to the path to the xemu binary
1. Set `GDB Server args` to `-s -S` (the `-S` is optional and will cause xemu to wait for the debugger to connnect)

To capture DbgPrint, additionally append `-device lpc47m157 -serial tcp:127.0.0.1:9091` to `GDB Server args` and use
something like [pykdclient](https://github.com/abaire/pykdclient).

## Deploying with xbdm_gdb_bridge

The `Makefile` contains a `deploy` target that will copy the finished binary to an XBOX running XBDM. This functionality
requires the [xbdm_gdb_bridge](https://github.com/abaire/xbdm_gdb_bridge) utility.