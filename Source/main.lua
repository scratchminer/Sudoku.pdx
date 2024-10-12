import("puzzle")

local coro = coroutine.create(function()
	local sec, ms = playdate.getSecondsSinceEpoch()
	local result, solution = Puzzle.generate(sec * 1000 + ms)
	
	result:print()
end)

--playdate.display.setRefreshRate(30)
playdate.setMinimumGCTime(2)

function playdate.update()
	if coroutine.status(coro) ~= "dead" then
		local s, err = coroutine.resume(coro)
		if not s then 
			error(err)
		end
		if coroutine.status(coro) == "dead" then
			print("time: " .. playdate.getCurrentTimeMilliseconds() / 1000 .. " seconds")
		end
	end
	playdate.drawFPS(0, 0)
end

