local M = 2000000
local flags = {}
local t = os.clock()
local count = 0
for rep = 0, 9 do
  for i = 0, M - 1 do flags[i] = 1 end
  count = 0
  for i = 2, M - 1 do
    if flags[i] ~= 0 then
      count = count + 1
      local j = i + i
      while j < M do
        flags[j] = 0
        j = j + i
      end
    end
  end
end
print("sieve", count, os.clock() - t)
