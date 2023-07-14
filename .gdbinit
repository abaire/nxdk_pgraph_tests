# CLion
# To allow this file to work you'll need to enable parsing of project-local
# .gdbinit files. Create/edit ~/.gdbinit (on WSL make sure it's inside the
# linux guest) and add:
#
# set auto-load local-gdbinit on
# add-auto-load-safe-path /


# Tell GDB that we are using 32-bit x86 architecture
set arch i386

# Use a layout which shows source code
# CLion: Not supported
#layout src

# Prevent CLion from dying with "doesn't support qGetTIBAddr packet"
set osabi none

# CLion: Handled by CLion
# Connect to the XQEMU GDB server
# target remote 127.0.0.1:1234

# Stop execution at the beginning of the `main` function
# b main

# Let XQEMU run until the CPU tries to execute from the address
# we have placed our breakpoint(s)
# c
