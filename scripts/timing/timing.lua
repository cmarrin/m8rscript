--
-- Timing test
--

local a = { }
local n = 4000
a.n = n

print("\n\nLua timing test: " .. n .. " squared iterations\n");

local startTime = os.clock();

for i = 0, n, 1 do
    for j = 0, n, 1 do
        local f = 3;
        a[j] = j * (j + 1) / 2;
    end
end

local t = os.clock() - startTime
print("Run time: " .. (t * 1000) .. "ms\n\n")
