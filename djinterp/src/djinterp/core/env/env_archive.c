/******************************************************************************
* djinterp [utility]                                             env_archive.c
*
*   Runtime implementations for the query declarations in env_archive.h. The
* notable one is d_env_archive_has_tool, a cross-platform PATH probe used to
* confirm an external archiver (e.g. "rar", "7z", "tar") is present before the
* archive layer shells out to it. The OS split is driven by the env.h flags.
*
* 
* path:      /src/djinterp/core/env/env_archive.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.23
******************************************************************************/
#include "../../../../inc/djinterp/core/env/env_archive.h"
#include "../../../../inc/djinterp/core/env/env_compress_link.h"


// d_env_archive_backend_name
//   function: human-readable name for a backend identifier.
const char*
d_env_archive_backend_name(int backend)
{
    switch (backend)
    {
        case D_ENV_ARCHIVE_BACKEND_LIBARCHIVE: return "libarchive";
        case D_ENV_ARCHIVE_BACKEND_LIBZIP:     return "libzip";
        case D_ENV_ARCHIVE_BACKEND_MINIZIP_NG: return "minizip-ng";
        case D_ENV_ARCHIVE_BACKEND_MINIZIP:    return "minizip";
        case D_ENV_ARCHIVE_BACKEND_MINIZ:      return "miniz";
        case D_ENV_ARCHIVE_BACKEND_LIBTAR:     return "libtar";
        case D_ENV_ARCHIVE_BACKEND_LZMA_SDK:   return "lzma-sdk";
        case D_ENV_ARCHIVE_BACKEND_BIT7Z:      return "bit7z";
        case D_ENV_ARCHIVE_BACKEND_UNRAR:      return "unrar";
        case D_ENV_ARCHIVE_BACKEND_RAR_TOOL:   return "rar-tool";
        case D_ENV_ARCHIVE_BACKEND_BUILTIN:    return "builtin";
        case D_ENV_ARCHIVE_BACKEND_NONE:       return "none";
        default:                               return "none";
    }
}

// d_env_archive_libarchive_runtime_version
//   function: libarchive runtime version string, or "unavailable".
const char*
d_env_archive_libarchive_runtime_version(void)
{
#if D_ENV_ARCHIVE_HAVE_LIBARCHIVE
    return archive_version_string();
#else
    return "unavailable";
#endif
}

// internal_tool_exists_at
//   function: test whether joining `dir` and `tool` (optionally with a
// Windows executable suffix) names an accessible file.
static int
internal_tool_exists_at(const char* dir,
                        size_t      dir_len,
                        const char* tool,
                        const char* suffix)
{
    char        path[2048];
    size_t      tool_len = strlen(tool);
    size_t      suf_len  = suffix ? strlen(suffix) : 0u;
    size_t      need;

    /* dir + separator + tool + suffix + NUL */
    need = dir_len + 1u + tool_len + suf_len + 1u;
    if (need > sizeof(path))
    {
        return 0;
    }

    memcpy(path, dir, dir_len);
    path[dir_len] = D_INTERNAL_DIR_SEP;
    memcpy(path + dir_len + 1u, tool, tool_len);
    if (suf_len > 0u)
    {
        memcpy(path + dir_len + 1u + tool_len, suffix, suf_len);
    }
    path[dir_len + 1u + tool_len + suf_len] = '\0';

    return (D_INTERNAL_ACCESS(path) == 0) ? 1 : 0;
}

// d_env_archive_has_tool
//   function: probe PATH for an executable named `tool_name`. returns 1 if
// found and executable, 0 otherwise. on Windows, common executable suffixes
// are tried when the name carries no extension.
int
d_env_archive_has_tool(const char* tool_name)
{
    const char* path_env;
    const char* cursor;

    if ((tool_name == NULL) || (tool_name[0] == '\0'))
    {
        return 0;
    }

    /* an explicit path (contains a separator) is tested directly */
    if (strchr(tool_name, '/') != NULL ||
        strchr(tool_name, '\\') != NULL)
    {
        return (D_INTERNAL_ACCESS(tool_name) == 0) ? 1 : 0;
    }

    path_env = getenv("PATH");
    if (path_env == NULL)
    {
        return 0;
    }

    cursor = path_env;
    while (*cursor != '\0')
    {
        const char* end = strchr(cursor, D_INTERNAL_PATH_SEP);
        size_t      len = (end != NULL) ? (size_t)(end - cursor)
                                        : strlen(cursor);

        if (len > 0u)
        {
#if D_INTERNAL_ARCHIVE_OS_WINDOWS
            /* try bare name, then .exe / .com / .bat */
            if (internal_tool_exists_at(cursor, len, tool_name, NULL)    ||
                internal_tool_exists_at(cursor, len, tool_name, ".exe")  ||
                internal_tool_exists_at(cursor, len, tool_name, ".com")  ||
                internal_tool_exists_at(cursor, len, tool_name, ".bat"))
            {
                return 1;
            }
#else
            if (internal_tool_exists_at(cursor, len, tool_name, NULL))
            {
                return 1;
            }
#endif
        }

        if (end == NULL)
        {
            break;
        }
        cursor = end + 1;
    }

    return 0;
}

// d_env_archive_print_info
//   function: print detected backends, versions, and the capability matrix.
void
d_env_archive_print_info(void)
{
    printf("djinterp archive backends:\n");

    printf("  libarchive : %s",
           D_ENV_ARCHIVE_HAVE_LIBARCHIVE ? "yes" : "no");
#if D_ENV_ARCHIVE_HAVE_LIBARCHIVE
    printf(" (runtime %s)", d_env_archive_libarchive_runtime_version());
#endif
    printf("\n");

    printf("  libzip     : %s\n",
           D_ENV_ARCHIVE_HAVE_LIBZIP ? "yes" : "no");
    printf("  minizip    : %s\n",
           D_ENV_ARCHIVE_HAVE_MINIZIP ? "yes" : "no");
    printf("  libtar     : %s\n",
           D_ENV_ARCHIVE_HAVE_LIBTAR ? "yes" : "no");
    printf("  lzma-sdk   : %s\n",
           D_ENV_ARCHIVE_HAVE_LZMA_SDK ? "yes" : "no");
    printf("  bit7z      : %s\n",
           D_ENV_ARCHIVE_HAVE_BIT7Z ? "yes" : "no");
    printf("  unrar lib  : %s\n",
           D_ENV_ARCHIVE_HAVE_UNRAR ? "yes" : "no");
    printf("  builtin tar/zip: %s / %s\n",
           D_ENV_ARCHIVE_HAVE_BUILTIN_TAR ? "yes" : "no",
           D_ENV_ARCHIVE_HAVE_BUILTIN_ZIP ? "yes" : "no");

    printf("  tools on PATH: rar=%d unrar=%d 7z=%d tar=%d\n",
           d_env_archive_has_tool("rar"),
           d_env_archive_has_tool("unrar"),
           d_env_archive_has_tool("7z"),
           d_env_archive_has_tool("tar"));

    printf("  zip   : read=%d write=%d\n",
           D_ENV_ARCHIVE_CAN_READ_ZIP, D_ENV_ARCHIVE_CAN_WRITE_ZIP);
    printf("  tar   : read=%d write=%d\n",
           D_ENV_ARCHIVE_CAN_READ_TAR, D_ENV_ARCHIVE_CAN_WRITE_TAR);
    printf("  gz    : read=%d write=%d\n",
           D_ENV_ARCHIVE_CAN_READ_GZ, D_ENV_ARCHIVE_CAN_WRITE_GZ);
    printf("  tar.gz: write=%d\n",
           D_ENV_ARCHIVE_CAN_WRITE_TGZ);
    printf("  7z    : read=%d write=%d\n",
           D_ENV_ARCHIVE_CAN_READ_7Z, D_ENV_ARCHIVE_CAN_WRITE_7Z);
    printf("  rar   : read=%d write=%d\n",
           D_ENV_ARCHIVE_CAN_READ_RAR, D_ENV_ARCHIVE_CAN_WRITE_RAR);

    return;
}