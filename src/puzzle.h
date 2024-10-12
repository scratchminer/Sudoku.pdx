#ifndef __SUDOKU_PUZZLE_H
#define __SUDOKU_PUZZLE_H

typedef struct {
	int numSquares;
	int *squares;
} PuzzleUnit;

typedef struct Puzzle {
	int boxSize;
	int size;
	int numDigits;
	int allDigits;
	
	int numSquares;
	int *squares;
	
	PuzzleUnit **units;
	
	int numAllUnits;
	PuzzleUnit *allUnits;
	
	int numPeersPerSquare;
	int **peers;
	
	int nextIndex; // used for flattening out the solution algorithm
	int choicesPos;
	int *choices;
} Puzzle;

typedef enum {
	kNoSolution,
	kOneSolution,
	kRecursiveSolution
} PuzzleSolveResult;

Puzzle *puzzle_init(int boxSize);
void puzzle_free(Puzzle *puz);

int puzzle_setSquare(Puzzle *puz, int id, int square);
int puzzle_getSquare(Puzzle *puz, int id);

PuzzleSolveResult puzzle_solveSingles(Puzzle *puz, Puzzle **solution);
PuzzleSolveResult puzzle_solveConjugates(Puzzle *puz, Puzzle **solution);
PuzzleSolveResult puzzle_solveHiddens(Puzzle *puz, Puzzle **solution);
PuzzleSolveResult puzzle_solveIntersection(Puzzle *puz, Puzzle **solution);
//PuzzleSolveResult puzzle_solveXWing(Puzzle *puz, Puzzle **solution);

PuzzleSolveResult puzzle_solveBruteForce(Puzzle *puz, Puzzle **solution);
void puzzle_solveRewind(Puzzle *puz);

Puzzle *puzzle_copy(Puzzle *puz);

#endif