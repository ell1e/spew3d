
# spew3d

You've found spew3d, a one-header file retro 3d toolkit for C.

**Features:** this library works best if you want potato low
poly-count graphics, a really simple API, and maybe even
procedural textures or geometry. Best for graphics mimicking
the original PlayStation, Nintendo 64, or similar consoles, and
copies some features like depth-sorting or partially-affine mapping.
Needs you to use & link [SDL2](https://libsdl.org),
includes image loading, physics, 3d animation, sound, and more.
*Since it uses SDL2's `SDL_Renderer` purely, it can run in pure
software rendering. No 3d acceleration needed.*

**Do not use if:** don't use this if you want modern shader effects,
modern realtime shadows, high poly count or giant levels with large
view range, or generally the best performance a 3d toolkit can give
you. It'll have *bad performance with high poly counts*, we promise!
Stick with its intended use and it'll run fine even on slow machines.


## Compiling / Usage

If you're using this directly from the repository, run `make` in the
main folder to prepare the resulting `spew3d.h` file with everything.

Then, put this in all your files where you want to use it from:

```
#include <spew3d.h>`
```

And put it only like this into one single object file, which will then
contain the actual implementation code and not just the header:

```
#define SPEW3D_IMPLEMENTATION
#include <spew3d.h>
#undef SPEW3D_IMPLEMENTATION
```

When you link your final program, make sure to add [SDL2](
https://libsdl.org) to the command line, and everything that SDL2
needs.

