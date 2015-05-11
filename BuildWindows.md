You can build UDPT on Windows with Visual Studio, MinGW or Cygwin.
The project is currently not supporting Visual Studio Makefiles, but you can use the Makefile with Cygwin or MinGW (MSYS).

## UDPT with MinGW (MSYS) or Cygwin ##
Building UDPT with MinGW (MSYS) or with Cygwin, is the same like [building on linux](BuildLinux.md); So you should continue to that article once you've got these tools: gcc, g++, make, libsqlite3-dev, git.

You may find it easier to build UDPT on Cygwin since it has a nice package manager.

**Note:** When compiling with Cygwin or MinGW, you may require other libraries that belong to MinGW/Cygwin. To resolve such a problem, before entering the `make` command, type: `$ export LDFLAGS="-static"`. This statically link required libraries to the executable file but it will take a bit more space.

## UDPT with Visual Studio ##
It is not recommended to build UDPT with Visual Studio unless you know what you are doing - The project currently does not support Visual Studio.

To build UDPT with Visual Studio, you can create a solution and add all the files under the "src" directory to it. Click build and it should work (make sure the directory structure is the same...).