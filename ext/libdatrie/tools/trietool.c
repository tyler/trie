/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * trietool.c - Trie manipulation tool
 * Created: 2006-08-15
 * Author:  Theppitak Karoonboonyanan <thep@linux.thai.net>
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include <config.h>
#include <datrie/sb-trie.h>

typedef struct {
    const char *path;
    const char *trie_name;
    SBTrie     *sb_trie;
} ProgEnv;

static int  decode_switch       (int argc, char *argv[], ProgEnv *env);
static int  decode_command      (int argc, char *argv[], ProgEnv *env);

static int  command_add         (int argc, char *argv[], ProgEnv *env);
static int  command_add_list    (int argc, char *argv[], ProgEnv *env);
static int  command_delete      (int argc, char *argv[], ProgEnv *env);
static int  command_delete_list (int argc, char *argv[], ProgEnv *env);
static int  command_query       (int argc, char *argv[], ProgEnv *env);
static int  command_list        (int argc, char *argv[], ProgEnv *env);

static void usage               (const char *prog_name, int exit_status);

static char *string_trim        (char *s);

int
main (int argc, char *argv[])
{
    int     i;
    ProgEnv env;
    int     ret;

    env.path = ".";

    i = decode_switch (argc, argv, &env);
    if (i == argc)
        usage (argv[0], EXIT_FAILURE);

    env.trie_name = argv[i++];
    env.sb_trie = sb_trie_open (env.path, env.trie_name,
                                TRIE_IO_READ | TRIE_IO_WRITE | TRIE_IO_CREATE);
    if (!env.sb_trie) {
        fprintf (stderr, "Cannot open trie '%s' at '%s'\n",
                 env.trie_name, env.path);
        exit (EXIT_FAILURE);
    }

    ret = decode_command (argc - i, argv + i, &env);

    sb_trie_close (env.sb_trie);

    return ret;
}

static int
decode_switch (int argc, char *argv[], ProgEnv *env)
{
    int opt_idx;

    for (opt_idx = 1; opt_idx < argc && *argv[opt_idx] == '-'; opt_idx++) {
        if (strcmp (argv[opt_idx], "-h") == 0 ||
            strcmp (argv[opt_idx], "--help") == 0)
        {
            usage (argv[0], EXIT_FAILURE);
        } else if (strcmp (argv[opt_idx], "-V") == 0 ||
                   strcmp (argv[opt_idx], "--version") == 0)
        {
            printf ("%s\n", VERSION);
            exit (EXIT_FAILURE);
        } else if (strcmp (argv[opt_idx], "-p") == 0 ||
                   strcmp (argv[opt_idx], "--path") == 0)
        {
            env->path = argv[++opt_idx];
        } else if (strcmp (argv[opt_idx], "--") == 0) {
            ++opt_idx;
            break;
        } else {
            fprintf (stderr, "Unknown option: %s\n", argv[opt_idx]);
            exit (EXIT_FAILURE);
        }
    }

    return opt_idx;
}

static int
decode_command (int argc, char *argv[], ProgEnv *env)
{
    int opt_idx;

    for (opt_idx = 0; opt_idx < argc; opt_idx++) {
        if (strcmp (argv[opt_idx], "add") == 0) {
            ++opt_idx;
            opt_idx += command_add (argc - opt_idx, argv + opt_idx, env);
        } else if (strcmp (argv[opt_idx], "add-list") == 0) {
            ++opt_idx;
            opt_idx += command_add_list (argc - opt_idx, argv + opt_idx, env);
        } else if (strcmp (argv[opt_idx], "delete") == 0) {
            ++opt_idx;
            opt_idx += command_delete (argc - opt_idx, argv + opt_idx, env);
        } else if (strcmp (argv[opt_idx], "delete-list") == 0) {
            ++opt_idx;
            opt_idx += command_delete_list (argc - opt_idx, argv + opt_idx, env);
        } else if (strcmp (argv[opt_idx], "query") == 0) {
            ++opt_idx;
            opt_idx += command_query (argc - opt_idx, argv + opt_idx, env);
        } else if (strcmp (argv[opt_idx], "list") == 0) {
            ++opt_idx;
            opt_idx += command_list (argc - opt_idx, argv + opt_idx, env);
        } else {
            fprintf (stderr, "Unknown command: %s\n", argv[opt_idx]);
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

static int
command_add (int argc, char *argv[], ProgEnv *env)
{
    int opt_idx;

    opt_idx = 0;
    while (opt_idx < argc) {
        const TrieChar *key;
        TrieData        data;

        key = (const TrieChar *) argv[opt_idx++];
        data = (opt_idx < argc) ? atoi (argv[opt_idx++]) : TRIE_DATA_ERROR;

        if (!sb_trie_store (env->sb_trie, key, data)) {
            fprintf (stderr, "Failed to add entry '%s' with data %d\n",
                     key, data);
        }
    }

    return opt_idx;
}

static int
command_add_list (int argc, char *argv[], ProgEnv *env)
{
    FILE   *input;
    char    line[256];

    input = fopen (argv[0], "r");
    if (!input) {
        fprintf (stderr, "add-list: Cannot open input file '%s'\n", argv[0]);
        return 1;
    }

    while (fgets (line, sizeof line, input)) {
        char       *key, *data;
        TrieData    data_val;

        key = string_trim (line);
        if ('\0' != *key) {
            /* find key boundary */
            for (data = key; *data && !strchr ("\t,", *data); ++data)
                ;
            /* mark key ending and find data begin */
            if ('\0' != *data) {
                *data++ = '\0';
                while (isspace (*data))
                    ++data;
            }
            /* decode data */
            data_val = ('\0' != *data) ? atoi (data) : TRIE_DATA_ERROR;

            /* store the key */
            if (!sb_trie_store (env->sb_trie, (const TrieChar *) key, data_val))
                fprintf (stderr, "Failed to add key '%s' with data %d.\n",
                         key, data_val);
        }
    }

    fclose (input);

    return 1;
}

static int
command_delete (int argc, char *argv[], ProgEnv *env)
{
    int opt_idx;

    for (opt_idx = 0; opt_idx < argc; opt_idx++)
        if (!sb_trie_delete (env->sb_trie, (const TrieChar *) argv[opt_idx]))
            fprintf (stderr, "No entry '%s'. Not deleted.\n", argv[opt_idx]);

    return opt_idx;
}

static int
command_delete_list (int argc, char *argv[], ProgEnv *env)
{
    FILE   *input;
    char    line[256];

    input = fopen (argv[0], "r");
    if (!input) {
        fprintf (stderr, "delete-list: Cannot open input file '%s'\n", argv[0]);
        return 1;
    }

    while (fgets (line, sizeof line, input)) {
        char   *p;

        p = string_trim (line);
        if ('\0' != *p)
            if (!sb_trie_delete (env->sb_trie, (const TrieChar *) p))
                fprintf (stderr, "No entry '%s'. Not deleted.\n", p);
    }

    fclose (input);

    return 1;
}

static int
command_query (int argc, char *argv[], ProgEnv *env)
{
    TrieData    data;

    if (argc == 0) {
        fprintf (stderr, "query: No key specified.\n");
        return 0;
    }

    if (sb_trie_retrieve (env->sb_trie, (const TrieChar *) argv[0], &data)) {
        printf ("%d\n", data);
    } else {
        fprintf (stderr, "query: Key '%s' not found.\n", argv[0]);
    }

    return 1;
}

static Bool
list_enum_func (const SBChar *key, TrieData key_data, void *user_data)
{
    printf ("%s\t%d\n", key, key_data);
    return TRUE;
}

static int
command_list (int argc, char *argv[], ProgEnv *env)
{
    sb_trie_enumerate (env->sb_trie, list_enum_func, (void *) 0);
    return 0;
}


static void
usage (const char *prog_name, int exit_status)
{
    printf ("%s - double-array trie manipulator\n", prog_name);
    printf ("Usage: %s [OPTION]... TRIE CMD ARG ...\n", prog_name);
    printf (
        "Options:\n"
        "  -p, --path DIR           set trie directory to DIR [default=.]\n"
        "  -h, --help               display this help and exit\n"
        "  -V, --version            output version information and exit\n"
        "\n"
        "Commands:\n"
        "  add  WORD DATA ...       add WORD with DATA to trie\n"
        "  add-list LISTFILE        add WORD and DATA from LISTFILE to trie\n"
        "  delete WORD ...          delete WORD from trie\n"
        "  delete-list LISTFILE     delete words listed in LISTFILE from trie\n"
        "  query WORD               query WORD data from trie\n"
        "  list                     list all words in trie\n"
    );

    exit (exit_status);
}

static char *
string_trim (char *s)
{
    char   *p;

    /* skip leading white spaces */
    while (*s && isspace (*s))
        ++s;

    /* trim trailing white spaces */
    p = s + strlen (s) - 1;
    while (isspace (*p))
        --p;
    *++p = '\0';

    return s;
}

/*
vi:ts=4:ai:expandtab
*/
