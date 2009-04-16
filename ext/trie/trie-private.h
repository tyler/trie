/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * trie-private.h - Private utilities for trie implementation
 * Created: 2007-08-25
 * Author:  Theppitak Karoonboonyanan <thep@linux.thai.net>
 */

#ifndef __TRIE_PRIVATE_H
#define __TRIE_PRIVATE_H

#include <datrie/typedefs.h>

/**
 * @file trie-private.h
 * @brief Private utilities for trie implementation
 */

/**
 * @brief Minimum value macro
 */
#define MIN_VAL(a,b)  ((a)<(b)?(a):(b))
/**
 * @brief Maximum value macro
 */
#define MAX_VAL(a,b)  ((a)>(b)?(a):(b))

#endif  /* __TRIE_PRIVATE_H */

/*
vi:ts=4:ai:expandtab
*/
