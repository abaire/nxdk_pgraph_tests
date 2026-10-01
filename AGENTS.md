# Agents.md

## Setup

- Install Python 3
- Install `nv2a-vsh` from Pypi (`pip install nv2a-vsh`)
- Install llvm, lld, bison, flex, clang-format

## Building

- Always build via cmake (e.g. `cmake --build cmake-build-debug`)

## Testing

- Testing this repository requires access to a physical Microsoft Xbox. This will not be available to agents. Ensure
  that changes build and allow the user to perform final testing.

## Code style

- Assume readers are expert C/C++ programmers and avoid unnecessary/obvious comments.
- Run `git clang-format` before any submission
- Add doccomments for all tests, including `@tc <test_name>` tags describing each test case and its expected output.

## On-Screen Text Layout (pbkit)

- The text overlay grid (`pbkit_print.c`) is fixed at **16 rows** (indices `0` to `15`) and **60 columns** (indices `0` to `59`).
- Pixel positioning metrics:
  - Column `c`: $X = 20 + 10 \times c$ (8-pixel glyph + 2-pixel right padding).
  - Row `r`: $Y = 25 + 25 \times r$ (16-pixel glyph height).
- `pb_printat(row, col, ...)` ignores row coordinates $\ge 16$ or column coordinates $\ge 60$, retaining previous cursor state.
- Exceeding column 59 or writing a newline (`\n`) advances `pb_next_row`. When `pb_next_row >= 16`, `pb_scrollup()` is called, which **discards row 0 and scrolls the entire screen up**.
- Do not append trailing `\n` to `pb_printat` calls. Always ensure string length fits within available columns (`col + len <= 60`).
- Plan screen geometry coordinates (640x480 framebuffer) around text row/column spans to prevent primitives from overlapping text.

## Texture Generation

- Always prefer direct texture generation functions (e.g., `GenerateRGBACheckerboard`, `GenerateSwizzledRGBACheckerboard`, `GenerateRGBATestPattern`, `GenerateSwizzledRGBATestPattern`, etc.) that write directly into texture memory (`host_.GetTextureMemory()`, `host_.GetTextureMemoryForStage(...)`).
- Do not use SDL surface versions of texture generator functions (e.g., `GenerateCheckerboardSurface`, `GenerateSurface`, `GenerateColoredCheckerboardSurface`) unless explicitly required.

