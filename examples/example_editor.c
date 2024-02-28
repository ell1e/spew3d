
//#define DEBUG_SPEW3D_EVENT
#define SPEW3D_DEBUG_OUTPUT
#define SPEW3D_IMPLEMENTATION
#include <spew3d.h>
#undef SPEW3D_IMPLEMENTATION
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

int main(int argc, const char **argv) {
    if (argc < 2 || strlen(argv[1]) < strlen(".map") ||
            memcmp(argv[1] + strlen(argv[1]) - strlen(".map"),
                ".map", strlen(".map")) != 0) {
        printf("Please specify a level file as first argument! "
            "Like this: example_editor my_little_area.map\n");
        return 1;
    }

    // First, ensure we're in the right folder:
    int _exists = 0;
    if (!spew3d_fs_TargetExists("grass.png", &_exists) || !_exists) {
        fprintf(stderr, "You didn't run this in 'examples' folder, "
            "or there was an I/O error.\n");
        return 1;
    }

    // Create a new window:
    s3d_window *win = spew3d_window_New(
        "Spew 3D Editor Example", 0
    );
    if (!win) {
        fprintf(stderr, "Spew3D initialization failed.\n");
        return 1;
    }
    spew3d_window_SetMouseLockMode(
        win, S3D_MOUSE_LOCK_INVISIBLE_RELATIVE_MODE
    );

    // Create the scene itself:
    s3d_scene3d *scene = spew3d_scene3d_New(100, 3.0);
    if (!scene) {
        failed: ;
        fprintf(stderr, "Failed to create scene or level.\n");
        return 1;
    }

    // Set up a level:
    s3d_lvlbox *level_contents = NULL;
    s3d_obj3d *level = NULL;
    s3d_resourceload_job *job = spew3d_lvlbox_FromMapFileOrNew(
        argv[1], 0, 1, "grass", 0
    );
    if (!job) goto failed;

    // Set up a camera:
    s3d_obj3d *camera = spew3d_camera3d_CreateForScene(
        scene
    );
    if (!camera) goto failed;
    spew3d_camera3d_SetFOV(camera, 100);

    // Add some random cube:
    s3d_geometry *cube_geo = spew3d_geometry_Create();
    if (!cube_geo) goto failed;
    if (!spew3d_geometry_AddCubeSimple(
            cube_geo, 2.0 * S3D_METER,
            spew3d_texture_FromFile("hello_world.png", 0), 0
        )) goto failed;
    s3d_obj3d *cube = spew3d_scene3d_AddMeshObj(scene, cube_geo, 1);
    if (!cube) goto failed;
    s3d_pos cube_pos = {0};
    cube_pos.x = 3 * S3D_METER;
    spew3d_obj3d_SetPos(cube, cube_pos);

    s3d_geometry *plane_geo = spew3d_geometry_Create();
    if (!plane_geo) goto failed;
    if (!spew3d_geometry_AddPlaneSimple(
            plane_geo, 4.0 * S3D_METER, 4.0 * S3D_METER, 1,
            spew3d_texture_FromFile("grass.png", 0), 0
            )) {
        goto failed;
    }
    s3d_obj3d *plane = spew3d_scene3d_AddMeshObj(scene, plane_geo, 1);
    if (!plane) goto failed;
    s3d_pos plane_pos = {0};
    plane_pos.z = -1 * S3D_METER;
    spew3d_obj3d_SetPos(plane, plane_pos);

    // Enter main loop:
    printf("Entering main loop.\n");
    uint64_t start_ticks = spew3d_time_Ticks();
    uint64_t move_ts = start_ticks;
    int notquit = 1;
    while (notquit) {
        spew3d_event_UpdateMainThread();

        // Process events:
        s3d_event e = {0};
        while (spew3d_event_q_Pop(spew3d_event_GetMainQueue(), &e)) {
            if (e.kind == S3DEV_WINDOW_USER_CLOSE_REQUEST ||
                    e.kind == S3DEV_APP_QUIT_REQUEST) {
                notquit = 0;
                break;
            }
            if (e.kind == S3DEV_MOUSE_MOVE) {
                s3d_rotation rot = spew3d_obj3d_GetRotation(camera);
                rot.hori += e.mouse.rel_x * 0.5;
                rot.verti -= e.mouse.rel_y * 0.5;
                spew3d_obj3d_SetRotation(camera, rot);
            }
        }
        // Move camera:
        while (move_ts < spew3d_time_Ticks()) {
            if (spew3d_keyboard_IsKeyPressed(
                    spew3d_window_GetID(win), S3D_KEY_W) ||
                    spew3d_keyboard_IsKeyPressed(
                    spew3d_window_GetID(win), S3D_KEY_UP)) {
                s3d_pos pos = spew3d_obj3d_GetPos(camera);
                spew3d_math3d_advanceforward(
                    pos, 0.05, spew3d_obj3d_GetRotation(camera),
                    &pos
                );
                spew3d_obj3d_SetPos(camera, pos);
            } else if (spew3d_keyboard_IsKeyPressed(
                    spew3d_window_GetID(win), S3D_KEY_S) ||
                    spew3d_keyboard_IsKeyPressed(
                    spew3d_window_GetID(win), S3D_KEY_DOWN)) {
                s3d_pos pos = spew3d_obj3d_GetPos(camera);
                spew3d_math3d_advanceforward(
                    pos, -0.05, spew3d_obj3d_GetRotation(camera),
                    &pos
                );
                spew3d_obj3d_SetPos(camera, pos);
            }
            if (spew3d_keyboard_IsKeyPressed(
                    spew3d_window_GetID(win), S3D_KEY_D)) {
                s3d_pos pos = spew3d_obj3d_GetPos(camera);
                spew3d_math3d_advancesideward(
                    pos, 0.05, spew3d_obj3d_GetRotation(camera),
                    &pos
                );
                spew3d_obj3d_SetPos(camera, pos);
            } else if (spew3d_keyboard_IsKeyPressed(
                    spew3d_window_GetID(win), S3D_KEY_A)) {
                s3d_pos pos = spew3d_obj3d_GetPos(camera);
                spew3d_math3d_advancesideward(
                    pos, -0.05, spew3d_obj3d_GetRotation(camera),
                    &pos
                );
                spew3d_obj3d_SetPos(camera, pos);
            }
            if (spew3d_keyboard_IsKeyPressed(
                    spew3d_window_GetID(win), S3D_KEY_LEFT)) {
                s3d_rotation rot = spew3d_obj3d_GetRotation(camera);
                rot.hori -= 1;
                spew3d_obj3d_SetRotation(camera, rot);
            } else if (spew3d_keyboard_IsKeyPressed(
                    spew3d_window_GetID(win), S3D_KEY_RIGHT)) {
                s3d_rotation rot = spew3d_obj3d_GetRotation(camera);
                rot.hori += 1;
                spew3d_obj3d_SetRotation(camera, rot);
            }
            move_ts += 10;
        }

        // Process level loading:
        if (!level && job != NULL) {
            if (s3d_resourceload_IsDone(job)) {
                level_contents = spew3d_lvlbox_FromMapFileFinalize(job);
                job = NULL;  // This was destroyed by finalize call.
                if (!level_contents) {
                    spew3d_window_Destroy(win);
                    goto failed;
                }
                level = spew3d_scene3d_AddLvlboxObj(
                    scene, level_contents, 1
                );
                level_contents = NULL;  // Now owned by level object.
            }
        }

        // Render scene:
        spew3d_window_FillWithColor(win, 1, 1, 1);
        spew3d_camera3d_RenderToWindow(camera, win);
        spew3d_window_PresentToScreen(win);
    }
    spew3d_window_Destroy(win);

    printf("Shutting down\n");
    return 0;
}

