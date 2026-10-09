local t = os.clock()
local s = 0
for i = 0, 100000000 - 1 do
  s = s + (i & 7) * 3
end
print("loop", s, os.clock() - t)
