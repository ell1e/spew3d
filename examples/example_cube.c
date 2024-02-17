
#define DEBUG_SPEW3D_EVENT
#define SPEW3D_IMPLEMENTATION
#include <spew3d.h>
#undef SPEW3D_IMPLEMENTATION
#include <math.h>
#include <stdio.h>
#include <stdint.h>


int main(int argc, const char **argv) {
    printf("Initializing Spew3D with window and renderer\n");
    s3d_window *win = spew3d_window_New(
        "Spew 3D Cube Example", 0
    );
    if (!win) {
        fprintf(stderr, "Spew3D initialization failed.\n");
        return 1;
    }

    printf("Creating a cube.\n");
    s3d_geometry *cube = spew3d_geometry_Create();
    if (cube) {
        if (!spew3d_geometry_AddCubeSimple(
                cube, 1.0 * S3D_METER,
                spew3d_texture_FromFile("numberone.jpg", 0), 0
                )) {
            spew3d_geometry_Destroy(cube);
            cube = NULL;
        }
    }
    if (!cube) {
        fprintf(stderr, "Failed to create geometry.\n");
        return 1;
    }

    printf("Setting up scene and camera.\n");
    s3d_scene3d *scene = spew3d_scene3d_New(100);
    if (!scene) {
        fprintf(stderr, "Failed to create scene.\n");
        return 1;
    }
    s3d_obj3d *obj = spew3d_scene3d_AddMeshObj(cube, 1);
    if (!obj) {
        fprintf(stderr, "Failed to create object.\n");
        return 1;
    }
    cube = NULL;  // Geometry is now owned by the object.

    printf("Entering main loop.\n");
    int notquit = 1;
    while (notquit) {
        s3devent_UpdateMainThread();

        s3devent e = {0};
        while (s3devent_q_Pop(s3devent_GetMainQueue(), &e)) {
            if (e.type == S3DEV_WINDOW_USER_CLOSE_REQUEST ||
                    e.type == S3DEV_APP_QUIT_REQUEST) {
                notquit = 0;
                break;
            }
        }
        spew3d_window_PresentToScreen(win);
    }
    spew3d_window_Destroy(win);

    printf("Shutting down\n");
    return 0;
}

