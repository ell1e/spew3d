
# Spew3D

You've found **Spew3D**, a one-header file **retro 3d toolkit
for C.**

**Features:**

- simple API,
- 2d and 3d graphics, image loading, sound, and more,
- retro-style like PlayStation 1 or Nintendo64,
- filesystem, multi-byte strings, etc. wrapped cross-platform,
- wide platform support thanks to [SDL2](https://libsdl.org).

**Do not use if** it disturbs you that Spew3D:

- can't do modern shader effects,
- can't do modern realtime shadows,
- can't handle modern higher poly counts.


## Compiling / Usage

*(Get `spew3d.h` [from here](https://codeberg.org/ell1e/spew3d/releases).)*

**Step 1:** add `spew3d.h` into your project's code folder, and
put this in all your files where you want to use it from:

```
#include <spew3d.h>
```

**Step 2:** in only a single object file, add this define which
will make it contain the actual implementation code and not just its API:

```
#define SPEW3D_IMPLEMENTATION
#include <spew3d.h>
#undef SPEW3D_IMPLEMENTATION
```

**Step 3:** When you link your final program, make sure to add [SDL2](
https://libsdl.org) to your linked libraries.


## Documentation

For now, please refer to the header files themselves like
[spew3d_init.h](./include/spew3d_init.h),
[spew3d_texture.h](./include/spew3d_texture.h), etc.
and the ['examples' folder](./examples/) for documentation.


### Common Compilation Problems

**Question: Where is `spew3d.h`?**

*Answer: it's generated and not
directly in the repository, [see here](#compiling-usage).
If you want to get it from the repository,
check the [section on running tests](#run-tests).*


**Question: I am getting missing definitions for `fseeko64` or
`ftello64` on Linux, what's up with that?**

*Answer: You're likely including `spew3d.h` after something
that already included the `stdio.h` header but without the
flag for 64bit file support which Spew3D needs. To solve this,
either add `-D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE` to
your gcc or clang compiler flags for your project, or include
`spew3d.h` before whatever other header that pulls in `stdio.h`.*


**Question: I included this with `extern "C" {` in my C++
program via `g++` (or similar) and got tons of errors!**

*Answer: Currently, C++ is not supported. This is in part due
to some included dependencies like miniz not supporting it, sorry.*


## Options

Spew3D has some options which you can use via `-DSPEW3D_OPTION...`
with gcc/mingw, or defining them **before** including `spew3d.h`.
Always specify the same ones wherever you include it!

Available options:

- `SPEW3D_OPTION_DISABLE_DLLEXPORT`: If defined, Spew3D will
  not mark its functions for shared library symbol export.
  By default, it will.

- `SPEW3D_OPTION_DISABLE_SDL`: If defined, allows compiling
  with no need for SDL2 whatsoever. It will also disable all
  graphical functions and sound output, but all other functionality
  remains available. This includes image loading, audio decoding
  without actual playback, threading and file system helpers,
  the Virtual File System, and so on. Ideal for headless use!


## Run Tests

Currently, running the tests is only supported on Linux.
This will also generate the `spew3d.h` file if you checked out
the development version of Spew3D at `include/spew3d.h`.

To run the tests, install SDL2 and libcheck (the GNU unit
test library for C) system-wide, then use: `make test`


## License

Spew3D is licensed under a BSD license or an Apache 2
License, [see here for details](LICENSE.md).
It includes other projects baked in, see `vendor` folder in the
repository.

