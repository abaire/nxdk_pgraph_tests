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
* Y - Toggle saving of results and multi-frame rendering.
* Start - Enter a submenu or test.
* Back - Go up one menu or leave a test. If pressed on the root menu, exit the application.
* Black - Exit the application.

### Test configuration

Behavior can optionally be determined via a JSON configuration file loaded from the XBOX.

```json
{
  "settings": {
    "enable_progress_log": false,
    "disable_autorun": false,
    "enable_autorun_immediately": false,
    "enable_shutdown_on_completion": false,
    "enable_pgraph_region_diff": false,
    "output_directory_path": "e:/nxdk_pgraph_tests",
    "skip_tests_by_default": false
  },
  "test_suites": {}
}
```

In the default release build, the program will look for this file in the `output_directory_path` (
`e:/nxdk_pgraph_tests/nxdk_pgraph_tests_config.json`) and `d:\nxdk_pgraph_tests_config.json`, taking whichever is found
first.

When building from source, the `sample-config.json` file in the `resources` directory can be copied to
`resources/nxdk_pgraph_tests_config.json` and modified in order to change the default behavior of the final xiso.

#### Filtering test suites/cases

The `"test_suites"` section may be used to filter the set of tests.

For example, suppose the program contains four test suites: `Default suite`, `Unlisted suite`, `Skipped suite`, and
`Suite with skipped test`. Each test suite contains two tests, `TestOne` and `TestTwo`. Without any filtering, the menu
tree would be something like:

```
Run all and exit
Default suite
  TestOne
  TestTwo
Unlisted suite
  TestOne
  TestTwo
Skipped suite
  TestOne
  TestTwo
Suite with skipped test
  TestOne
  TestTwo
```

___Note___: The names used within `"test_suites"` and its children are the same as the names that appear in the
`nxdk_praph_tests` menu.

By default, any descendant of the `"test_suites"` object that contains a `"skipped": true` will be omitted.

For example, the following config will disable all tests except

```json
{
  "settings": {},
  "test_suites": {
    "Skipped suite": {
      "skipped": true,
      "TestOne": {}
    },
    "Suite with skipped test": {
      "TestTwo": {
        "skipped": true
      }
    },
    "Default suite": {
      "TestOne": {}
    }
  }
}
```

Resulting in

```
Run all and exit
Default suite
  TestOne
  TestTwo
Unlisted suite
  TestOne
  TestTwo
Suite with skipped test
  TestOne
```

This behavior may be modified via the `"skip_tests_by_default"` setting, allowing only entries with an explicit
`"skipped": false` to be retained. For example, the following config will disable all tests except the `TestOne` case
under `Default suite`.

```json
{
  "settings": {
    "skip_tests_by_default": true
  },
  "test_suites": {
    "Default suite": {
      "TestOne": {
        "skipped": true
      }
    }
  }
}
```

### Progress logging

If the `enable_progress_log` runtime config variable is set to `true`, a progress log named `pgraph_progress_log.txt`
will be saved in the output directory. This may be useful when trying to track down emulator crashes (e.g., due to
unimplemented features).

## Build prerequisites

This project uses [nv2a-vsh](https://pypi.org/project/nv2a-vsh/) to assemble some of the vertex shaders for tests.
It can be installed via `pip3 install nv2a-vsh --upgrade`.

This test suite requires some modifications to the pbkit used by the nxdk in order to operate.

To facilitate this, the nxdk is included as a submodule of this project, referencing the
`nxdk_pgraph_tester` branch from https://github.com/abaire/nxdk.

This project should be cloned with the `--recursive` flag to pull the submodules and their submodules,
after the fact this can be achieved via `git submodule update --init --recursive`.

As of July 2023, the nxdk's CMake implementation requires bootstrapping before it may be used. To facilitate this, run
the `prewarm-nxdk.sh` script from this project's root directory. It will navigate into the `nxdk` subdir and build all
of the sample projects, triggering the creation of the `nxdk` libraries needed for the toolchain and for this project.

### Generating the default JSON config

1. Configure CMake with the `DUMP_CONFIG_FILE` option set to ON.
2. Build and run the xiso.
3. Copy the new `sample-config.json` file from the output directory on the XBOX/emulator into the `resources` directory
   in this repo.

Note that, when running in `DUMP_CONFIG_FILE` mode, any existing `pgraph_test_config.json` file will be ignored.

### Building with CLion

The CMake target can be configured to use the toolchain from the nxdk:

* CMake options

  `-DCMAKE_TOOLCHAIN_FILE=<absolute_path_to_this_project>/third_party/nxdk/share/toolchain-nxdk.cmake`

* Environment

  `NXDK_DIR=<absolute_path_to_this_project>/third_party/nxdk`

On macOS you may also have to modify `PATH` in the `Environment` section such that a homebrew version of LLVM
is preferred over Xcode's (to supply `dlltool`).

#### Example settings

Assuming that this project has been checked out at `/development/pgraph_tester`:

* CMake options: `-DCMAKE_TOOLCHAIN_FILE=/development/pgraph_tester/third_party/nxdk/share/toolchain-nxdk.cmake`
* Environment: `NXDK_DIR=/development/pgraph_tester/third_party/nxdk`

## Adding new tests

Autogenerated docs may be accessed at https://abaire.github.io/nxdk_pgraph_tests

### Using nv2a log events from [xemu](https://xemu.app/)

1. Enable tracing of nv2a log events as normal (see xemu documentation) and exercise the event of interest within the
   game.
1. Duplicate an existing test as a skeleton.
1. Add the duplicated test to `CMakeLists.txt` and `main.cpp` (please preserve alphabetical ordering if possible).
1. Use [nv2a_to_pbkit](https://github.com/abaire/nv2a_to_pbkit) to get a rough set of pbkit invocations duplicating the
   behavior from the log. Take the interesting portions of the converted output and put them into the body of the test.
   You may wish to utilize some of the helper methods from `TestHost`   and similar classes rather than using the raw
   output to improve readability.

### Writing nv2a vertex shaders in assembly

See [the README in the nv2a_vsh_asm repository](https://github.com/abaire/nv2a_vsh_asm) for an overview.

`*.vsh` files are assembed via the `generate_nv2a_vshinc_files` function in CMakeLists.txt. Each vsh file produces a
corresponding `.vshinc` file that contains a C-style list of 32-bit integers containing the vertex shader operations.
This file is intended to be included from test source files to initialize a constant array which may then be used to
populate a `VertexShaderProgram` via `SetShaderOverride`.

For example:

```c++
// Note that the VertexShaderProgram does not copy this data, so it is important that it remain in scope throughout
// the use of the VertexShaderProgram object.
static const uint32_t kPassthroughVsh[] = {
#include "passthrough.vshinc"
};

// ...

  auto shader = std::make_shared<VertexShaderProgram>();
  shader->SetShaderOverride(kPassthroughVsh, sizeof(kPassthroughVsh));
  host_.SetVertexShaderProgram(shader);

```

Uniform values may then be set via the `VertexShaderProgram::SetUniform*` series of functions. By default, the 0th
uniform will be available in the shader as `c96`.

For example, a shader that takes a 4-element vertex might have the code:

```asm
#vertex vector 96
```

which would be populated via

```c++
  shader->shader->SetUniformF(0, -1.f, 1.5f, 0.f, 1.f);
```

## Running with CLion

### On xemu

Create a build target

1. Create a new `Embedded GDB Server` target
1. Set the Target to `nxdk_pgraph_tests_xiso`
1. Set the Executable to `nxdk_pgraph_tests`
1. Set `Download executable` to `Never`
1. Set `'target remote' args` to `127.0.0.1:1234`
1. Set `GDB Server` to the path to the xemu binary
1. Set `GDB Server args` to `-s -S -dvd_path "$CMakeCurrentBuildDir$/xiso/nxdk_pgraph_tests.iso"` (the `-S` is
   optional and will cause xemu to wait for the debugger to connnect)
1. Under `Advanced GDB Server Options`
    1. Set "Working directory" to `$ProjectFileDir$`
    1. On macOS, set "Environment variables"
       to
       `DYLD_FALLBACK_LIBRARY_PATH=/<the full path to your xemu.app bundle>/Contents/Libraries/<the architecture for your platform, e.g., arm64>`
    3. Set "Reset command" to `Never`

To capture DbgPrint, additionally append `-device lpc47m157 -serial tcp:127.0.0.1:9091` to `GDB Server args` and use
something like [pykdclient](https://github.com/abaire/pykdclient).

NOTE: If you see a failure due to "Remote target doesn't support qGetTIBAddr packet", check the GDB output to make sure
that the `.gdbinit` file was successfully loaded.

## Deploying with [xbdm_gdb_bridge](https://github.com/abaire/xbdm_gdb_bridge)

To create a launch configuration that deploys the devhost to an XBDM-enabled XBOX (devkit) with debugging enabled:

1. Upload the full set of resource files to the XBOX

   This can be done with [xbdm_gdb_bridge binary](https://github.com/abaire/xbdm_gdb_bridge) and a `CMake Application`
   target.
    1. Create a new `CMake Application` target
    2. Set `Target` to `nxdk_pgraph_tests_xiso`
    3. Set `Executable` to the full path to the [xbdm_gdb_bridge binary](https://github.com/abaire/xbdm_gdb_bridge)
       binary
    4. Set `Program arguments`
       to
       `<YOUR_XBOX_IP> -v3 -- mkdir e:\$CMakeCurrentTargetName$ && putfile $CMakeCurrentBuildDir$/xbe/xbe_file e:\$CMakeCurrentTargetName$ -f`
    5. Run the target. You will need to do this any time the resources are changed. The XBE can be uploaded as part of
       the debug step.

1. Create a new `Embedded GDB Server` run config.
    1. Set the "Target" to `nxdk_pgraph_tests`
    1. Set the "Executable binary" to `nxdk_pgraph_tests`
    1. Set "Download executable" to `Never`
    1. Set "'target remote' args" to `localhost:1999`
    1. Set "GDB Server" to the full path to the [xbdm_gdb_bridge binary](https://github.com/abaire/xbdm_gdb_bridge)
       binary
    1. Set "GDB Server args"
       to `<YOUR_XBOX_IP> -s -- gdb :1999 e:\$CMakeCurrentTargetName$`
    1. Under "Advanced GDB Server Options"
        1. Set "Reset command" to `Never`
    1. Add a "Before launch" > "Run External tool" to copy the default.xbe to the XBOX:
        1. Set "Program" to the full path to the [xbdm_gdb_bridge binary](https://github.com/abaire/xbdm_gdb_bridge)
           binary
        2. Set "Arguments"
           to
           `<YOUR_XBOX_IP> -- mkdir e:\$CMakeCurrentTargetName$ && putfile $CMakeCurrentBuildDir$/xbe/xbe_file/default.xbe e:\$CMakeCurrentTargetName$ -f`
        3. Uncheck "Synchronize files after execution"
