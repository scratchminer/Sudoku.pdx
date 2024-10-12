function Puzzle:print()
	for y = 1, self:getSize() do
		st = ""
		for x = 1, self:getSize() do
			local sq = self:getSquare(x, y)
			local ones = {}
			
			for i = 0, 8 do
				if (sq >> i) & 1 ~= 0 then ones[#ones + 1] = i end
			end
			
			local s
			if #ones == 1 then
				s = ones[1] + 1
			elseif #ones == 9 then
				s = "."
			else
				s = "{"
				for _, v in ipairs(ones) do
					s = s .. (v + 1)
				end
				s = s .. "}"
			end
			
			st = st .. s .. " "
		end
		print(st)
	end
end

function Puzzle.parse(numbers, size)
	local puz = Puzzle.new(size)
	
	for i = 0, puz:getSize() ^ 2 - 1 do
		local s = string.sub(numbers, i + 1, i + 1)
		puz:setSquare(i, s == "." and ((1 << puz:getSize()) - 1) or (1 << (tonumber(s) - 1)))
	end
	
	return puz
end

-- solves a puzzle using coroutines
function Puzzle:solvedPuzzle(force, maxLevel, hint)
	local puz = {self}
	
	self:rewind()
	
	maxLevel = maxLevel or 4
	
	local techniqueLevel = 0
	local maxLevelUsed = 0
	
	if not force then
		while techniqueLevel <= maxLevel do
			local result, solution = puz[#puz]:step(techniqueLevel)
			if maxLevelUsed < techniqueLevel then
				maxLevelUsed = techniqueLevel
			end
			
			if result == Puzzle.kNoSolution then
				return nil
			elseif result == Puzzle.kOneSolution then
				return solution, maxLevelUsed
			elseif result == Puzzle.kRecursiveSolution then
				if solution ~= puz[#puz] then
					if hint then
						return solution, techniqueLevel
					end
					techniqueLevel = 1
					puz[#puz + 1] = solution
				else
					techniqueLevel = techniqueLevel + 1
				end
			end
			
			coroutine.yield()
		end
	else
		while #puz > 0 do
			local result, solution = puz[#puz]:step(techniqueLevel)
			
			if result == Puzzle.kNoSolution then
				puz[#puz] = nil
			elseif result == Puzzle.kOneSolution then
				return solution, 0
			elseif result == Puzzle.kRecursiveSolution then
				if techniqueLevel ~= 0 and techniqueLevel < maxLevel then
					techniqueLevel = techniqueLevel + 1
				else
					techniqueLevel = 0
				end
				puz[#puz + 1] = solution
			end
			
			coroutine.yield()
		end
	end
	
	return nil
end

local function shuffle(tbl)
	for i = #tbl, 2, -1 do
		local j = math.random(i)
		tbl[i], tbl[j] = tbl[j], tbl[i]
	end
	return tbl
end

-- returns the puzzle with a hint applied
function Puzzle:hintPuzzle(solved, bigHint)
	solved = solved or self:solvedPuzzle(false, nil, true)
	local eliminations = {}
	local numEliminations = {}
	
	local newSquares = {}
	local numNewSquares = {}
	
	local hintPuz = self:copy()
	
	for id = 0, self:getSize() ^ 2 - 1 do
		local sq, sq2 = self:getSquare(id), solved:getSquare(id)
		if sq ~= sq2 then
			local ones = {}
			
			for i = 0, self:getSize() - 1 do
				local digit = 1 << i
				if sq2 & digit ~= 0 then
					ones[#ones + 1] = digit
				end
			end
			
			if #ones == 1 then
				newSquares[id] = sq2
				numNewSquares[#numNewSquares + 1] = id
			else
				eliminations[id] = ones
				numEliminations[#numEliminations + 1] = id
			end
		end
	end
	
	if bigHint and #numNewSquares > 0 then
		shuffle(numNewSquares)
		hintPuz:setSquare(numNewSquares[1], newSquares[numNewSquares[1]])
	elseif #numEliminations > 0 then
		shuffle(numEliminations)
		shuffle(eliminations[numEliminations[1]])
		hintPuz:setSquare(numEliminations[1], sq | eliminations[numEliminations[1]][1])
	end
	
	return hintPuz
end

-- returns a freshly generated puzzle and its solution
function Puzzle.generate(seed, difficulty, size)
	if type(seed) == "number" then
		Puzzle.setSeed(seed)
		math.randomseed(seed)
	end
	
	print("creating full grid...")
	
	size = size or 3
	difficulty = difficulty or 3
	
	local puz = Puzzle.new(size)
	local result = puz:solvedPuzzle(true)
	local workingPuz = result:copy()
	
	print("full grid solved, creating holes...")
	
	local numbers = table.create(puz:getSize() ^ 2, 0)
	for i = 1, puz:getSize() ^ 2 do
		numbers[i] = i - 1
	end
	shuffle(numbers)
	
	while #numbers > 0 do
		local id = numbers[#numbers]
		
		local sq = workingPuz:getSquare(id)
		workingPuz:setSquare(id, (1 << workingPuz:getSize()) - 1)
		
		local solved, maxUsed = workingPuz:solvedPuzzle(false, difficulty)
		if solved == nil then
			workingPuz:setSquare(id, sq)
		else
			if maxUsed < difficulty - 1 then
				-- ???
			end
		end
		
		numbers[#numbers] = nil
	end
	
	return workingPuz, result
end