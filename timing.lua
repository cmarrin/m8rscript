--
-- Timing test
--

local a = { }
local n = 1000
a.n = n

local startTime = os.clock();

for i = 0, n, 1 do
    for j = 0, n, 1 do
        a[j] = j * (j + 1) / 2;
    end
end

local t = os.clock() - startTime
print("Run time: " .. (t * 1000) .. "ms\n")
