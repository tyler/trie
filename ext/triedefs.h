/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * triedefs.h - General typedefs for trie
 * Created: 2006-08-11
 * Author:  Theppitak Karoonboonyanan <thep@linux.thai.net>
 */

#ifndef __TRIEDEFS_H
#define __TRIEDEFS_H

#include <datrie/typedefs.h>

/**
 * @file triedefs.h
 * @brief General typedefs for trie
 */

/**
 * @brief Trie IO modes
 */
typedef enum {
    TRIE_IO_READ   = 0x01,
    TRIE_IO_WRITE  = 0x02,
    TRIE_IO_CREATE = 0x04
} TrieIOMode;

/**
 * @brief Trie character type for alphabet
 */
typedef uint32         AlphaChar;

/**
 * @brief Error value for alphabet character
 */
#define ALPHA_CHAR_ERROR   (~(AlphaChar)0)

/**
 * @brief Trie character type for key
 */
typedef unsigned char  TrieChar;
/**
 * @brief Trie terminator character
 */
#define TRIE_CHAR_TERM    '\0'
#define TRIE_CHAR_MAX     255

/**
 * @brief Type of Trie index
 */
typedef int32          TrieIndex;
/**
 * @brief Trie error index
 */
#define TRIE_INDEX_ERROR  0
/**
 * @brief Maximum trie index value
 */
#define TRIE_INDEX_MAX    0x7fffffff

/**
 * @brief Type of value associated to trie entries
 */
typedef int32          TrieData;
/**
 * @brief Trie error data
 */
#define TRIE_DATA_ERROR  -1

#endif  /* __TRIEDEFS_H */

/*
vi:ts=4:ai:expandtab
*/
