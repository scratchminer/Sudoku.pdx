#include <stdlib.h>
#include <pd_api.h>

extern PlaydateAPI *pd;

#include "luaglue.h"
#include "puzzle.h"

int luapuzzle_setSeed(lua_State *L) {
	int argc = pd->lua->getArgCount();
	if (argc == 0) {
		unsigned int sec, ms;
		sec = pd->system->getSecondsSinceEpoch(&ms);
		srand(sec * 1000 + ms);
	}
	
	return 0;
}

int luapuzzle_new(lua_State *L) {
	int boxSize;
	int argc = pd->lua->getArgCount();
	
	if (argc == 0) {
		boxSize = 3;
	}
	else {
		boxSize = pd->lua->getArgInt(1);
	}
	
	Puzzle *puz = puzzle_init(boxSize);	
	
	pd->lua->pushObject(puz, "Puzzle", 0);
	
	return 1;
}

int luapuzzle_setSquare(lua_State *L) {
	int id, value;
	int argc = pd->lua->getArgCount();
	
	Puzzle *puz = pd->lua->getArgObject(1, "Puzzle", NULL);
	if (puz == NULL) {
		pd->system->error("Puzzle.setSquare() requires a Puzzle as the first argument");
		return 0;
	}
	else if (argc < 3) {
		pd->system->error("Puzzle:setSquare() requires at least two arguments");
		return 0;
	}
	else if (argc == 3) {
		id = pd->lua->getArgInt(2);
		value = pd->lua->getArgInt(3);
		
		puzzle_setSquare(puz, id, value);
		return 0;
	}
	else {
		id = ((pd->lua->getArgInt(3) - 1) * puz->size) + pd->lua->getArgInt(2) - 1;
		value = pd->lua->getArgInt(4);
		
		puzzle_setSquare(puz, id, value);
		return 0;
	}
}

int luapuzzle_getSquare(lua_State *L) {
	int id;
	int argc = pd->lua->getArgCount();
	
	Puzzle *puz = pd->lua->getArgObject(1, "Puzzle", NULL);
	if (puz == NULL) {
		pd->system->error("Puzzle.getSquare() requires a Puzzle as the first argument");
		return 0;
	}
	else if (argc < 2) {
		pd->system->error("Puzzle:getSquare() requires at least one argument");
		return 0;
	}
	else if (argc == 2) {
		id = pd->lua->getArgInt(2);
		
		pd->lua->pushInt(puzzle_getSquare(puz, id));
		return 1;
	}
	else {
		id = ((pd->lua->getArgInt(3) - 1) * puz->size) + pd->lua->getArgInt(2) - 1;
		
		pd->lua->pushInt(puzzle_getSquare(puz, id));
		return 1;
	}
}

int luapuzzle_getSize(lua_State *L) {
	Puzzle *puz = pd->lua->getArgObject(1, "Puzzle", NULL);
	if (puz == NULL) {
		pd->system->error("Puzzle.step() requires a Puzzle as the first argument");
		return 0;
	}
	
	pd->lua->pushInt(puz->size);
	return 1;
}

int luapuzzle_solveStep(lua_State *L) {
	Puzzle *puz = pd->lua->getArgObject(1, "Puzzle", NULL);
	if (puz == NULL) {
		pd->system->error("Puzzle.step() requires a Puzzle as the first argument");
		return 0;
	}
	
	int step = 0;
	if (pd->lua->getArgCount() >= 2) {
		step = pd->lua->getArgInt(2);
	}
	
	Puzzle *solution;
	PuzzleSolveResult result;
	
	switch (step) {
		case 1: result = puzzle_solveSingles(puz, &solution); break;
		case 2: result = puzzle_solveConjugates(puz, &solution); break;
		case 3: result = puzzle_solveHiddens(puz, &solution); break;
		case 4: result = puzzle_solveIntersection(puz, &solution); break;
		case 0: default: result = puzzle_solveBruteForce(puz, &solution); break;
	}
	
	pd->lua->pushInt(result);
	
	if (solution != NULL) {
		pd->lua->pushObject(solution, "Puzzle", 0);
	}
	else {
		pd->lua->pushNil();
	}
	
	return 2;
}

int luapuzzle_solveRewind(lua_State *L) {
	Puzzle *puz = pd->lua->getArgObject(1, "Puzzle", NULL);
	if (puz == NULL) {
		pd->system->error("Puzzle.rewind() requires a Puzzle as the first argument");
		return 0;
	}
	
	puzzle_solveRewind(puz);
	return 0;
}

int luapuzzle_copy(lua_State *L) {
	Puzzle *puz = pd->lua->getArgObject(1, "Puzzle", NULL);
	if (puz == NULL) {
		pd->system->error("Puzzle.copy() requires a Puzzle as the first argument");
		return 0;
	}
	
	Puzzle *copy = puzzle_copy(puz);
	pd->lua->pushObject(copy, "Puzzle", 0);
	
	return 1;
}

int luapuzzle_eq(lua_State *L) {
	Puzzle *puz = pd->lua->getArgObject(1, "Puzzle", NULL);
	if (puz == NULL) {
		pd->system->error("Puzzle.__eq() requires a Puzzle as the first argument");
		return 0;
	}
	
	Puzzle *puz2 = pd->lua->getArgObject(2, "Puzzle", NULL);
	if (puz2 == NULL) {
		pd->system->error("puzzle:__eq() requires a Puzzle as the first argument");
		return 0;
	}
	
	if (puz->size != puz2->size) {
		pd->lua->pushBool(0);
		return 1;
	}
	
	for (int id = 0; id < puz->numSquares; id++) {
		if (puzzle_getSquare(puz, id) != puzzle_getSquare(puz2, id)) {
			pd->lua->pushBool(0);
			return 1;
		}
	}
	
	pd->lua->pushBool(1);
	return 1;
}

int luapuzzle_gc(lua_State *L) {
	Puzzle *puz = pd->lua->getArgObject(1, "Puzzle", NULL);
	if (puz == NULL) {
		pd->system->error("Puzzle.__gc() requires a Puzzle as the first argument");
		return 0;
	}
	
	puzzle_free(puz);
	
	return 1;
}