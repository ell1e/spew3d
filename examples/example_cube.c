
#define DEBUG_SPEW3D_EVENT
#define SPEW3D_DEBUG_OUTPUT
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

    printf("Creating geometry.\n");
    s3d_geometry *cube_geo = spew3d_geometry_Create();
    if (!cube_geo) goto failed;
    if (!spew3d_geometry_AddCubeSimple(
            cube_geo, 2.0 * S3D_METER,
            spew3d_texture_FromFile("hello_world.png", 0), 0
            )) {
        failed:
        fprintf(stderr, "Failed to create scene.\n");
        return 1;
    }
    s3d_geometry *plane_geo = spew3d_geometry_Create();
    if (!plane_geo) goto failed;
    if (!spew3d_geometry_AddPlaneSimple(
            plane_geo, 4.0 * S3D_METER, 4.0 * S3D_METER, 1,
            spew3d_texture_FromFile("grass.png", 0), 0
            )) {
        goto failed;
    }

    printf("Setting up scene and camera.\n");
    s3d_scene3d *scene = spew3d_scene3d_New(100, 3.0);
    if (!scene) goto failed;
    s3d_obj3d *cube = spew3d_scene3d_AddMeshObj(scene, cube_geo, 1);
    if (!cube) goto failed;
    s3d_obj3d *plane = spew3d_scene3d_AddMeshObj(scene, plane_geo, 1);
    if (!plane) goto failed;
    s3d_pos plane_pos = {0};
    plane_pos.z = -1 * S3D_METER;
    spew3d_obj3d_SetPos(plane, plane_pos);
    cube_geo = NULL;  // Geometry is now owned by the objects.
    plane_geo = NULL;

    // Set up a camera:
    s3d_obj3d *camera = spew3d_camera3d_CreateForScene(
        scene
    );
    if (!camera) goto failed;
    s3d_pos cam_pos = {0};
    cam_pos.x = -3 * S3D_METER;
    cam_pos.z = 0.5 * S3D_METER;
    spew3d_obj3d_SetPos(camera, cam_pos);

    // Enter main loop:
    printf("Entering main loop.\n");
    uint64_t start_ticks = spew3d_time_Ticks();
    int notquit = 1;
    while (notquit) {
        s3devent_UpdateMainThread();

        // Process events:
        s3devent e = {0};
        while (s3devent_q_Pop(s3devent_GetMainQueue(), &e)) {
            if (e.kind == S3DEV_WINDOW_USER_CLOSE_REQUEST ||
                    e.kind == S3DEV_APP_QUIT_REQUEST) {
                notquit = 0;
                break;
            }
        }

        // Rotate things a little bit, for fun:
        double degree = (spew3d_time_Ticks() - start_ticks) / 100.0;
        s3d_rotation r = {0};
        r.hori = degree;
        r.roll = degree * 2;
        spew3d_obj3d_SetRotation(cube, r);
        s3d_rotation cam_rot = {0};
        cam_rot.hori = 0;
        //cam_rot.roll = degree * 1.2;
        spew3d_obj3d_SetRotation(camera, cam_rot);

        // Render scene:
        spew3d_window_FillWithColor(win, 0.15, 0.05, 0.0);
        spew3d_camera3d_RenderToWindow(camera, win);
        spew3d_window_PresentToScreen(win);
    }
    spew3d_window_Destroy(win);

    printf("Shutting down\n");
    return 0;
}

