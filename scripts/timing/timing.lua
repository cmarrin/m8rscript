--
-- Timing test
--

local a = { }
local n = 10
local loops = 500
a.n = n

print("\n\nLua timing test: " .. loops .. " squared iterations\n");

local startTime = os.clock();

for i = 1, loops, 1 do
    for j = 1, loops, 1 do
        local f = 3;
        a[5] = j * (j + 1) / 2;
    end
end

local t = os.clock() - startTime;
print("Run time: " .. t .. "ms\n\n");
