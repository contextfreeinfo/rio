local N = 10000
local ps = {}
for i = 0, N - 1 do ps[i] = {x = i, y = 0.0, vx = 1.0, vy = (i % 7) + 0.0} end
local t = os.clock()
for step = 0, 999 do
  for i = 0, N - 1 do
    local p = ps[i]
    p.vy = p.vy - 0.01
    p.x = p.x + p.vx * 0.016
    p.y = p.y + p.vy * 0.016
    if p.y < 0 then
      p.y = -p.y
      p.vy = -p.vy * 0.9
    end
  end
end
local sum = 0.0
for i = 0, N - 1 do sum = sum + ps[i].y end
print("particles", sum, os.clock() - t)
