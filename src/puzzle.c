#include <stdlib.h>
#include <pd_api.h>

extern PlaydateAPI *pd;

#include "puzzle.h"

Puzzle *puzzle_init(int boxSize) {
	int size = boxSize * boxSize;
	
	Puzzle *puz = pd->system->realloc(NULL, sizeof(Puzzle));
	
	puz->boxSize = boxSize;
	puz->size = size;
	
	puz->numDigits = size;
	puz->allDigits = (1 << puz->numDigits) - 1;
	
	puz->numSquares = size * size;
	puz->squares = pd->system->realloc(NULL, puz->numSquares * sizeof(int));
	
	puz->units = pd->system->realloc(NULL, puz->numSquares * sizeof(PuzzleUnit *));
	puz->peers = pd->system->realloc(NULL, puz->numSquares * sizeof(int *));
	
	puz->numPeersPerSquare = (boxSize * boxSize - 1) + (size - boxSize) * 2;
	
	for (int i = 0; i < puz->numSquares; i++) {
		puz->squares[i] = puz->allDigits;
		puz->units[i] = pd->system->realloc(NULL, 3 * sizeof(PuzzleUnit));
		
		puz->units[i][0].numSquares = size;
		puz->units[i][0].squares = pd->system->realloc(NULL, size * sizeof(int));
	
		puz->units[i][1].numSquares = size;
		puz->units[i][1].squares = pd->system->realloc(NULL, size * sizeof(int));
		
		int x = i % size;
		int y = i / size;
		
		for (int j = 0; j < size; j++) {
			puz->units[i][0].squares[j] = y * size + j;
			puz->units[i][1].squares[j] = j * size + x;
		}
		
		puz->units[i][2].numSquares = size;
		puz->units[i][2].squares = pd->system->realloc(NULL, size * sizeof(int));
		
		int boxX = (x / boxSize) * boxSize;
		int boxY = (y / boxSize) * boxSize;
		
		for (int j = 0; j < boxSize; j++) {
			for (int k = 0; k < boxSize; k++) {
				puz->units[i][2].squares[j * boxSize + k] = (boxY + j) * size + (boxX + k);
			}
		}
		
		puz->peers[i] = pd->system->realloc(NULL, puz->numPeersPerSquare * sizeof(int));
		int actualPeers = 0;
		int sq, found;
		
		for (int j = 0; j < 3; j++) {
			for (int k = 0; k < puz->units[i][j].numSquares; k++) {
				sq = puz->units[i][j].squares[k];
				found = 0;
				
				for (int l = 0; l < actualPeers; l++) {
					if (puz->peers[i][l] == sq) {
						found = 1;
						break;
					}
				}
				
				if ((found == 0) && (sq != i)) puz->peers[i][actualPeers++] = sq;
			}
		}
	}
	
	puz->numAllUnits = 0;
	puz->allUnits = pd->system->realloc(NULL, (3 * size) * sizeof(PuzzleUnit));
	
	for (int j = 0; j < size; j++) {
		memcpy(&puz->allUnits[puz->numAllUnits++], &puz->units[j * size][0], sizeof(PuzzleUnit));
		memcpy(&puz->allUnits[puz->numAllUnits++], &puz->units[j][1], sizeof(PuzzleUnit));
	}
	for (int j = 0; j < puz->numSquares; j += size * boxSize) {
		for (int k = 0; k < size; k += boxSize) {
			memcpy(&puz->allUnits[puz->numAllUnits++], &puz->units[j + k][2], sizeof(PuzzleUnit));
		}
	}
	
	puz->nextIndex = -1;
	puz->choicesPos = -1;
	puz->choices = pd->system->realloc(NULL, puz->numDigits * sizeof(int));
	
	return puz;
}

void puzzle_free(Puzzle *puz) {
	for (int i = 0; i < puz->numSquares; i++) {
		pd->system->realloc(puz->units[i][0].squares, 0);
		pd->system->realloc(puz->units[i][1].squares, 0);
		pd->system->realloc(puz->units[i][2].squares, 0);
		pd->system->realloc(puz->units[i], 0);
		
		pd->system->realloc(puz->peers[i], 0);
	}
	
	pd->system->realloc(puz->units, 0);
	pd->system->realloc(puz->peers, 0);
	
	pd->system->realloc(puz->allUnits, 0);
	
	pd->system->realloc(puz->squares, 0);
	pd->system->realloc(puz->choices, 0);
	pd->system->realloc(puz, 0);
}

int puzzle_setSquare(Puzzle *puz, int id, int square) {
	if ((id > puz->numSquares) || (id < 0)) {
		pd->system->error("puzzle_setSquare: square ID out of range: %d", id);
		return -1;
	}
	else {
		return puz->squares[id] = square;
	}
}

int puzzle_getSquare(Puzzle *puz, int id) {
	if ((id > puz->numSquares) || (id < 0)) {
		pd->system->error("puzzle_getSquare: square ID out of range: %d", id);
		return -1;
	}
	else {
		return puz->squares[id];
	}
}

Puzzle *puzzle_copy(Puzzle *puz) {
	Puzzle *newPuz = puzzle_init(puz->boxSize);
	memcpy(newPuz->squares, puz->squares, newPuz->numSquares * sizeof(int));
	
	return newPuz;
}

static void clear(Puzzle *puz) {
	for (int i = 0; i < puz->numSquares; i++) {
		puz->squares[i] = puz->allDigits;
	}
}

static inline int ones(Puzzle *puz, int digits) {
	int onesCount = 0;
	for (int i = 0; i < puz->numDigits; i++) {
		if ((digits & (1 << i)) != 0) onesCount++;
	}
	return onesCount;
}

static Puzzle *eliminate(Puzzle *puz, int id, int digit);

static Puzzle *fill(Puzzle *puz, int id, int digit) {
	int sq = puzzle_getSquare(puz, id);
	if (sq == digit) {
		return puz;
	}
	int digit2;
	
	for (int i = 0; i < puz->numDigits; i++) {
		digit2 = 1 << i;
		if (((sq & digit2) != 0) && ((digit & digit2) == 0)) {
			if (eliminate(puz, id, digit2) == NULL) {
				return NULL;
			}
		}
	}
	
	return puz;
}

static Puzzle *eliminate(Puzzle *puz, int id, int digit) {
	int sq = puzzle_getSquare(puz, id);
	
	if ((sq & digit) == 0) {
		return puz;
	}
	sq = puzzle_setSquare(puz, id, sq & ~digit);
	
	int onesCount = ones(puz, sq);
	if (onesCount == 0) {
		// puzzle is unsolvable since no digit can fit into the given spot
		return NULL;
	}
	else if (onesCount == 1) {
		for (int i = 0; i < puz->numPeersPerSquare; i++) {
			if (eliminate(puz, puz->peers[id][i], sq) == NULL) {
				return NULL;
			}
		}
	}
	
	for (int i = 0; i < 3; i++) {
		int numPlaces = 0;
		int firstPlace = -1;
		
		for (int j = 0; j < puz->units[id][i].numSquares; j++) {
			if ((puzzle_getSquare(puz, puz->units[id][i].squares[j]) & digit) != 0) {
				numPlaces++;
				firstPlace = puz->units[id][i].squares[j];
			}
		}
		
		if ((numPlaces == 0) || ((numPlaces == 1) && (fill(puz, firstPlace, digit) == NULL))) {
			// puzzle is unsolvable since no digit can fit into the single open spot in the square's units
			return NULL;
		}
	}
	
	return puz;
}

static void shuffle(int *array, size_t n) {
	if (n > 1) {
		for (size_t i = 0; i < n - 1; i++) {
			size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
			int t = array[j];
			array[j] = array[i];
			array[i] = t;
		}
	}
}

// Methods to solve a puzzle:
// propagates solutions
static Puzzle *constrain(Puzzle *puz) {
	Puzzle *result = puzzle_copy(puz);
	clear(result);
	
	int sq;
	int onesCount;
	
	for (int id = 0; id < puz->numSquares; id++) {
		sq = puz->squares[id];
		onesCount = ones(puz, sq);
		
		if (onesCount == 1) {
			fill(result, id, sq);
		}
	}
	
	return result;
}

// 1. solves hidden singles
static Puzzle *singles(Puzzle *puz) {
	Puzzle *result = puzzle_copy(puz);
	
	int id;
	int numTimes;
	int firstTime;
	
	for (int i = 0; i < puz->numAllUnits; i++) {
		for (int j = 0; j < puz->numDigits; j++) {
			numTimes = 0;
			
			for (int k = 0; k < puz->allUnits[i].numSquares; k++) {
				id = puz->allUnits[i].squares[j];
				if (puz->squares[id] == j) {
					numTimes++;
					firstTime = id;
				}
			}
			
			if (numTimes == 1) {
				fill(result, firstTime, 1 << j);
			}
		}
	}
	
	return result;
}

// 2. solves conjugate pairs and triples
static Puzzle *conjugates(Puzzle *puz) {
	Puzzle *result = puzzle_copy(puz);
	
	int id1, id2, id3;
	int sq1, sq2, sq3;
	int id;
	
	for (int i = 0; i < puz->numAllUnits; i++) {
		for (int j = 0; j < puz->allUnits[i].numSquares; j++) {
			id1 = puz->allUnits[i].squares[j];
			sq1 = puz->squares[id1];
			
			for (int k = j + 1; k < puz->allUnits[i].numSquares; k++) {
				id2 = puz->allUnits[i].squares[k];
				sq2 = puz->squares[id2];
				
				if ((ones(puz, sq1 | sq2) == 2) && (ones(puz, sq1) > 1) && (ones(puz, sq2) > 1)) {
					for (int l = 0; l < puz->numDigits; l++) {
						if (((sq1 | sq2) & (1 << l)) != 0) {
							for (int m = 0; m < puz->allUnits[i].numSquares; m++) {
								id = puz->allUnits[i].squares[m];
								
								if ((id != id1) && (id != id2)) {
									eliminate(result, id, 1 << l);
								}
							}
						}
					}
				}
				else {
					for (int l = k + 1; l < puz->allUnits[i].numSquares; l++) {
						id3 = puz->allUnits[i].squares[l];
						sq3 = puz->squares[id3];
						
						if ((ones(puz, sq1 | sq2 | sq3) == 3) && (ones(puz, sq1) > 1) && (ones(puz, sq2) > 1) && (ones(puz, sq3) > 1)) {
							for (int m = 0; m < puz->numDigits; m++) {
								if (((sq1 | sq2 | sq3) & (1 << m)) != 0) {
									for (int n = 0; n < puz->allUnits[i].numSquares; n++) {
										id = puz->allUnits[i].squares[n];
										
										if ((id != id1) && (id != id2) && (id != id3)) {
											eliminate(result, id, 1 << m);
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	
	return result;
}

// 3. solves hidden pairs and triples
static Puzzle *hiddens(Puzzle *puz) {
	Puzzle *result = puzzle_copy(puz);
	
	int id;
	int times[puz->size];
	int numTimes;
	int digits;
	
	for (int i = 0; i < puz->numAllUnits; i++) {
		for (int j = 0; j < puz->numDigits; j++) {
			for (int k = j + 1; k < puz->numDigits; k++) {
				digits = (1 << j) | (1 << k); 
				numTimes = 0;
				
				for (int l = 0; l < puz->allUnits[i].numSquares; l++) {
					id = puz->allUnits[i].squares[l];
					
					if (ones(puz, puz->squares[id] & digits) == 2) {
						times[numTimes++] = id;
					}
					else if (ones(puz, puz->squares[id] & digits) > 0) {
						numTimes = 0;
						break;
					}
				}
				
				if (numTimes == 2) {
					for (int l = 0; l < puz->numDigits; l++) {
						if ((l != j) && (l != k)) {
							eliminate(result, times[0], 1 << l);
							eliminate(result, times[1], 1 << l);
						}
					}
				}
				else {
					for (int l = k + 1; l < puz->numDigits; l++) {
						digits = (1 << j) | (1 << k) | (1 << l);
						numTimes = 0;
				
						for (int m = 0; m < puz->allUnits[i].numSquares; m++) {
							id = puz->allUnits[i].squares[m];
							if (ones(puz, puz->squares[id] & digits) >= 2) {
								times[numTimes++] = id;
							}
							else if (ones(puz, puz->squares[id] & digits) > 0) {
								numTimes = 0;
								break;
							}
						}
						
						if (numTimes == 3) {
							for (int m = 0; m < puz->numDigits; m++) {
								if ((m != j) && (m != k) && (m != l)) {
									eliminate(result, times[0], 1 << m);
									eliminate(result, times[1], 1 << m);
									eliminate(result, times[2], 1 << m);
								}
							}
						}
					}
				}
			}
		}
	}
	
	return result;
}

// 4. solves by intersection removal
static Puzzle *intersection(Puzzle *puz) {
	Puzzle *result = puzzle_copy(puz);
	
	int digit;
	int times[puz->size];
	int numTimes;
	int row, col;
	int collinear;
	
	for (int i = 0; i < puz->numAllUnits; i++) {
		for (int j = 0; j < puz->numDigits; j++) {
			digit = 1 << j;
			numTimes = 0;
			
			for (int k = 0; k < puz->allUnits[i].numSquares; k++) {
				int id = puz->allUnits[i].squares[k];
				if ((puz->squares[id] & digit) == digit) {
					times[numTimes++] = id;
				}
			}
			
			if ((numTimes > 1) && (numTimes <= puz->boxSize)) {
				row = -1;
				col = -1;
				collinear = -1;
				
				for (int k = 0; k < numTimes; k++) {
					if (i < (2 * puz->size)) {
						int r = times[k] / puz->size;
						int c = times[k] % puz->size;
					
						if (k == 0) {
							row = r;
							col = c;
						}
						else {
							if ((r != row) && (c == col)) {
								if (collinear == 0) {
									break;
								}
								else {
									collinear = 1;
								}
							}
							else if ((r == row) && (c != col)) {
								if (collinear == 1) {
									break;
								}
								else {
									collinear = 0;
								}
							}
							else {
								break;
							}
						}
					}
					else {
						int r = (times[k] / puz->size) / puz->boxSize * puz->boxSize;
						int c = (times[k] % puz->size) / puz->boxSize * puz->boxSize;
						
						if (k == 0) {
							row = r;
							col = c;
						}
						else {
							if ((r != row) || (c != col)) {
								break;
							}
							else if (collinear == -1) {
								collinear = 2;
							}
						}
					}
				}
				
				if (collinear >= 0) {
					int found;
					
					if (collinear == 0) {
						for (int k = row * puz->size; k < (row + 1) * puz->size; k++) {
							found = 0;
							for (int l = 0; l < numTimes; l++) {
								if (times[l] == k) {
									found = 1;
									break;
								}
							}
						
							if (found == 0) {
								eliminate(result, k, digit);
							}
						}
					}
					else if (collinear == 1) {
						for (int k = col; k < puz->numSquares; k += puz->size) {
							found = 0;
							for (int l = 0; l < numTimes; l++) {
								if (times[l] == k) {
									found = 1;
									break;
								}
							}
						
							if (found == 0) {
								eliminate(result, k, digit);
							}
						}
					}
					else if (collinear == 2) {
						int id;
						
						for (int k = 0; k < puz->boxSize; k++) {
							for (int l = 0; l < puz->boxSize; l++) {
								id = (row + k) * puz->size + col + l;
								found = 0;
								
								for (int m = 0; m < numTimes; m++) {
									if (times[m] == id) {
										found = 1;
										break;
									}
								}
								
								if (found == 0) {
									eliminate(result, id, digit);
								}
							}
						}
					}
				}
			}
		}
	}
	
	return result;
}

static PuzzleSolveResult search(Puzzle *puz) {
	if (puz == NULL) {
		return kNoSolution;
	}
	
	int minLength = puz->numDigits + 1;
	int minIndex = -1;
	
	int sq;
	int onesCount;
	
	for (int id = 0; id < puz->numSquares; id++) {
		sq = puz->squares[id];
		onesCount = 0;
		
		for (int i = 0; i < puz->numDigits; i++) {
			if ((sq & (1 << i)) != 0) ++onesCount;
		}
		
		if ((onesCount > 1) && (onesCount < minLength)) {
			minIndex = id;
			minLength = onesCount;
		}
	}
	
	if (minIndex == -1) {
		puz->nextIndex = -1;
		puz->choicesPos = -1;
		return kOneSolution;
	}
	else {
		sq = puz->squares[minIndex];
		onesCount = 0;
		
		for (int i = 0; i < puz->numDigits; i++) {
			if ((sq & (1 << i)) != 0) puz->choices[onesCount++] = (1 << i);
		}
		shuffle(puz->choices, minLength);
		
		puz->nextIndex = minIndex;
		puz->choicesPos = minLength - 1;
		return kRecursiveSolution;
	}
}

PuzzleSolveResult puzzle_solveSingles(Puzzle *puz, Puzzle **solution) {
	*solution = singles(puz);
	return search(*solution);
}

PuzzleSolveResult puzzle_solveConjugates(Puzzle *puz, Puzzle **solution) {
	*solution = conjugates(puz);
	return search(*solution);
}

PuzzleSolveResult puzzle_solveHiddens(Puzzle *puz, Puzzle **solution) {
	*solution = hiddens(puz);
	return search(*solution);
}

PuzzleSolveResult puzzle_solveIntersection(Puzzle *puz, Puzzle **solution) {
	*solution = intersection(puz);
	return search(*solution);
}

PuzzleSolveResult puzzle_solveBruteForce(Puzzle *puz, Puzzle **solution) {
	if (puz->nextIndex == -1) {
		*solution = constrain(puz);
		return search(*solution);
	}
	else if (puz->choicesPos >= 0) {
		*solution = puzzle_copy(puz);
		return search(fill(*solution, puz->nextIndex, puz->choices[puz->choicesPos--]));
	}
	else {
		*solution = puz;
		return search(*solution);
	}
}

void puzzle_solveRewind(Puzzle *puz) {
	puz->nextIndex = -1;
	puz->choicesPos = -1;
}
