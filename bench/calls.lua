local function add3(a, b, c) return a + b + c end
local t = os.clock()
local s = 0
for i = 0, 30000000 - 1 do
  s = add3(s, i, 1) & 0xffffff
end
print("calls", s, os.clock() - t)
