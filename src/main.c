#include <stdint.h>
#include <stddef.h>
#include <pd_api.h>

PlaydateAPI *pd;

#include "luaglue.h"
#include "puzzle.h"

static const lua_reg puzzleLib[] = {
	{"setSeed", luapuzzle_setSeed},
	{"new", luapuzzle_new},
	{"setSquare", luapuzzle_setSquare},
	{"getSquare", luapuzzle_getSquare},
	{"getSize", luapuzzle_getSize},
	{"step", luapuzzle_solveStep},
	{"rewind", luapuzzle_solveRewind},
	{"copy", luapuzzle_copy},
	{"__eq", luapuzzle_eq},
	{"__gc", luapuzzle_gc},
	{NULL, NULL}
};

static const lua_val puzzleLibVals[] = {
	{"kNoSolution", kInt, {kNoSolution}},
	{"kOneSolution", kInt, {kOneSolution}},
	{"kRecursiveSolution", kInt, {kRecursiveSolution}},
	{NULL, kInt, {0}}
};

#ifdef _WINDLL
__declspec(dllexport)
#endif
int eventHandler(PlaydateAPI* playdate, PDSystemEvent event, uint32_t arg) {
	(void)arg;
	if (event == kEventInit) {
		pd = playdate;
	}
	else if (event == kEventInitLua) {
		pd->lua->registerClass("Puzzle", puzzleLib, puzzleLibVals, 0, NULL);
	}
	
	return 0;
}