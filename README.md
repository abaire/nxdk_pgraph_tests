nxdk_pgraph_tests
====

Various tests for nv2a rendering.

Based on code from https://github.com/Ernegien/nxdk/tree/test/texture-formats/samples/texture-formats

Golden test results from XBOX hardware are [available here](https://github.com/abaire/nxdk_pgraph_tests_golden_results)

## Usage

Tests will be executed automatically if no gamepad input is given within an initial timeout.

Individual tests may be executed via the menu.

### Controls

DPAD:

* Up - Move the menu cursor up. Inside of a test, go to the previous test in the active suite.
* Down - Move the menu cursor down. Inside of a test, go to the previous test in the active suite.
* Left - Move the menu cursor up by half a page.
* Right - Move the menu cursor down by half a page.
* A - Enter a submenu or test. Inside of a test, re-run the test.
* B - Go up one menu or leave a test. If pressed on the root menu, exit the application.
* X - Run all tests for the current suite.
* Start - Enter a submenu or test.
* Back - Go up one menu or leave a test. If pressed on the root menu, exit the application.
* Black - Exit the application.

## Build prerequisites

This test suite requires some modifications to the pbkit used by the nxdk in order to operate.

To facilitate this, the nxdk is included as a submodule of this project, referencing the
`pbkit_extensions` branch from https://github.com/abaire/nxdk.

This project should be cloned with the `--recursive` flag to pull the submodules and their submodules,
after the fact this can be achieved via `git submodule update --init --recursive`.

For macOS, the patched nxdk currently assumes that the Homebrew llvm@11 package has been installed.


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
