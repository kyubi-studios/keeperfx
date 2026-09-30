/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file game_legacy.c
 *     Module which contains the legacy Game structure.
 * @par Purpose:
 *     Allows easy saving and loading of game data.
 * @par Comment:
 *     None.
 * @author   Tomasz Lis
 * @date     21 Oct 2009 - 23 Nov 2012
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#include "pre_inc.h"
#include "game_legacy.h"

#include "globals.h"
#include "bflib_basics.h"
#include "post_inc.h"

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************/
#if defined(KFX_GAME_ON_HEAP)
struct Game *kfx_game_ptr = NULL;

/* Runs before main() and C++ static constructors, so nothing sees it unset. */
__attribute__((constructor(101))) static void kfx_allocate_game(void)
{
    kfx_game_ptr = (struct Game *)calloc(1, sizeof(struct Game));
    if (kfx_game_ptr == NULL)
        abort();
}
#else
struct Game game;
#endif

GameTurn get_gameturn()
{
    return game.play_gameturn;
}

TbBool network_is_active(void)
{
    return flag_is_set(game.system_flags, GSF_NetworkActive);
}
/******************************************************************************/
#ifdef __cplusplus
}
#endif
/******************************************************************************/
/******************************************************************************/

#include <stddef.h>
/* Subtile coordinates overlay 'val' (see KFX_STL_PACKED in globals.h). */
_Static_assert(offsetof(struct Coord3d, x.stl.num) == offsetof(struct Coord3d, x.val) + 1, "Coord3d stl.num must start at byte 1");
_Static_assert(offsetof(struct Coord2d, y.stl.num) == offsetof(struct Coord2d, y.val) + 1, "Coord2d stl.num must start at byte 1");
