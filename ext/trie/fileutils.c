/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * fileutils.h - File utility functions
 * Created: 2006-08-15
 * Author:  Theppitak Karoonboonyanan <thep@linux.thai.net>
 */

#include <string.h>
#include <stdlib.h>

#include "fileutils.h"

/*--------------------------------------*
 *    INTERNAL FUNCTIONS DECLARATIONS   *
 *--------------------------------------*/

static char *   make_full_path (const char *dir,
                                const char *name,
                                const char *ext);

/* ==================== BEGIN IMPLEMENTATION PART ====================  */

/*--------------------------------*
 *    FUNCTIONS IMPLEMENTATIONS   *
 *--------------------------------*/

static char *
make_full_path (const char *dir, const char *name, const char *ext)
{
    char   *path;

    path = (char *) malloc (strlen (dir) + strlen (name) + strlen (ext) + 2);
    sprintf (path, "%s/%s%s", dir, name, ext);

    return path;
}

FILE *
file_open (const char *dir, const char *name, const char *ext, TrieIOMode mode)
{
    const char *std_mode;
    char       *full_path;
    FILE       *file;

    if (mode & TRIE_IO_WRITE)
        std_mode = "r+";
    else
        std_mode = "r";

    full_path = make_full_path (dir, name, ext);
    file = fopen (full_path, std_mode);
    if (!file && mode & TRIE_IO_CREATE)
        file = fopen (full_path, "w+");
    free (full_path);

    return file;
}

long
file_length (FILE *file)
{
    long    cur_pos;
    long    size;

    cur_pos = ftell (file);

    fseek (file, 0L, SEEK_END);
    size = ftell (file);

    fseek (file, cur_pos, SEEK_SET);

    return size;
}

Bool
file_read_int32 (FILE *file, int32 *o_val)
{
    unsigned char   buff[4];

    if (fread (buff, 4, 1, file) == 1) {
        *o_val = (buff[0] << 24) | (buff[1] << 16) |  (buff[2] << 8) | buff[3];
        return TRUE;
    }

    return FALSE;
}

Bool
file_write_int32 (FILE *file, int32 val)
{
    unsigned char   buff[4];

    buff[0] = (val >> 24) & 0xff;
    buff[1] = (val >> 16) & 0xff;
    buff[2] = (val >> 8) & 0xff;
    buff[3] = val & 0xff;

    return (fwrite (buff, 4, 1, file) == 1);
}

Bool
file_read_int16 (FILE *file, int16 *o_val)
{
    unsigned char   buff[2];

    if (fread (buff, 2, 1, file) == 1) {
        *o_val = (buff[0] << 8) | buff[1];
        return TRUE;
    }

    return FALSE;
}

Bool
file_write_int16 (FILE *file, int16 val)
{
    unsigned char   buff[2];

    buff[0] = val >> 8;
    buff[1] = val & 0xff;

    return (fwrite (buff, 2, 1, file) == 1);
}

Bool
file_read_int8 (FILE *file, int8 *o_val)
{
    return (fread (o_val, sizeof (int8), 1, file) == 1);
}

Bool
file_write_int8 (FILE *file, int8 val)
{
    return (fwrite (&val, sizeof (int8), 1, file) == 1);
}

Bool
file_read_chars (FILE *file, char *buff, int len)
{
    return (fread (buff, sizeof (char), len, file) == len);
}

Bool
file_write_chars (FILE *file, const char *buff, int len)
{
    return (fwrite (buff, sizeof (char), len, file) == len);
}

/*
vi:ts=4:ai:expandtab
*/
