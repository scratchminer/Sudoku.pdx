#ifndef __SUDOKU_LUAGLUE_H
#define __SUDOKU_LUAGLUE_H

#include <pd_api.h>

int luapuzzle_setSeed(lua_State *L);

int luapuzzle_new(lua_State *L);

int luapuzzle_setSquare(lua_State *L);
int luapuzzle_getSquare(lua_State *L);

int luapuzzle_getSize(lua_State *L);

int luapuzzle_solveStep(lua_State *L);
int luapuzzle_solveRewind(lua_State *L);

int luapuzzle_copy(lua_State *L);

int luapuzzle_eq(lua_State *L);
int luapuzzle_gc(lua_State *L);

#endif