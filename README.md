
# Spew3D

You've found **Spew3D**, a one-header file **retro 3d toolkit for C.**

**Features:**

- potato low poly-count graphics,
- simple API,
- 2d and 3d graphics, image loading, sound, and more,
- good for procedural textures or geometry,
- retro-style like PlayStation 1 or Nintendo64,
- wide platform support thanks to [SDL2](https://libsdl.org).

**Do not use if** it disturbs you that Spew3D:

- can't do modern shader effects,
- can't do modern realtime shadows,
- can't handle modern higher poly counts.


## Compiling / Usage

*(If you're using this directly from the repository, run
`make update-vendor` and `make` in the repo folder to prepare
the resulting `spew3d.h` file with everything.)*

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
[spew3d_init.h](spew3d_init.h), [spew3d_texture.h](spew3d_texture.h),
and the `examples` folder for documentation.


### Common Compilation Problems

**Question: I am getting missing definitions for `fseeko64` or `ftello64` on Linux, what's up with that?**

*Answer: You're likely including `spew3d.h` after something that already included the `stdio.h` header but without the flag for 64bit file support which Spew3D needs. To solve this, either add `-D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE` to your gcc or clang compiler flags for your project, or include `spew3d.h` before whatever other header that pulls in `stdio.h`.*


## License

Spew3D is licensed under the MIT license, [see here](LICENSE.md).
It includes other projects baked in, see `vendor` folder in the repository.

