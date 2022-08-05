

#ifdef SPEW3D_IMPLEMENTATION

#include <stdint.h>
#include <stdio.h>


int spew3d_vfs_FileToBytes(
        const char *path,
        char **out_bytes,
        uint64_t *out_bytes_len
        ) {
    #if defined(DEBUG_SPEW3D_VFS)
    fprintf(stderr, "spew3d_vfs.c: debug: "
        "spew3d_vfs_FileToBytes on: \"%s\"\n",
        path);
    #endif

    // Try to open via VFS first:

    // Fall back to filesystem if not in VFS:
    #if defined(DEBUG_SPEW3D_VFS)
    fprintf(stderr, "spew3d_vfs.c: debug: "
        "spew3d_vfs_FileToBytes falling back to disk\n");
    #endif
    int ferr = 0;
    FILE *f = spew3d_fs_OpenFromPath(
        path, "rb", &ferr
    ); 
    if (!f) {
        #if defined(DEBUG_SPEW3D_VFS)
        fprintf(stderr, "spew3d_vfs.c: debug: "
            "spew3d_vfs_FileToBytes: disk file open failed\n");
        #endif
        return 0;
    }
    uint64_t filesize = 0;
    if (!spew3d_fs_GetSize(path, &filesize, &ferr)) {
        #if defined(DEBUG_SPEW3D_VFS)
        fprintf(stderr, "spew3d_vfs.c: debug: "
            "spew3d_vfs_FileToBytes: getting size failed\n");
        #endif
        fclose(f);
        return 0;
    } 
    char *result_bytes = malloc(
        (filesize > 0ULL ? filesize : 1ULL)
    );
    if (!result_bytes) {
        #if defined(DEBUG_SPEW3D_VFS)
        fprintf(stderr, "spew3d_vfs.c: debug: "
            "spew3d_vfs_FileToBytes: allocating buffer "
            "for file contents failed\n");
        #endif
        fclose(f);
        return 0;
    }
    char *p = result_bytes;
    uint64_t bytesread = 0;
    while (bytesread < filesize) {
        uint64_t readchunk = (filesize - bytesread);
        if (readchunk > 1024ULL * 1024ULL)
            readchunk = 1024ULL * 1024ULL;
        size_t result = fread(p, 1, readchunk, f);
        if ((uint64_t)result != readchunk) {
            #if defined(DEBUG_SPEW3D_VFS)
            fprintf(stderr, "spew3d_vfs.c: debug: "
                "spew3d_vfs_FileToBytes: got fread() error\n");
            #endif
            fclose(f);
            free(result_bytes);
            return 0;
        }
        p += (uint64_t)result;
        bytesread += (uint64_t)result;
    }
    *out_bytes = result_bytes;
    *out_bytes_len = filesize;
    #if defined(DEBUG_SPEW3D_VFS)
    fprintf(stderr, "spew3d_vfs.c: debug: "
        "spew3d_vfs_FileToBytes completed successfully\n");
    #endif
    return 1;
}


#endif  // SPEW_IMPLEMENTATION

