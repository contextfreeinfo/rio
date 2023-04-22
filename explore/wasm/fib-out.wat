(module
  (type (;0;) (func (param i32) (result i32)))
  (func (;0;) (type 0) (param i32) (result i32)
    (local i32)
    local.get 0
    i32.const 0
    i32.le_s
    if  ;; label = @1
      i32.const 0
      local.set 1
    else
      local.get 0
      i32.const 1
      i32.eq
      if  ;; label = @2
        i32.const 1
        local.set 1
      else
        local.get 0
        i32.const 2
        i32.sub
        call 0
        local.get 0
        i32.const 1
        i32.sub
        call 0
        i32.add
        local.set 1
      end
    end
    local.get 1)
  (memory (;0;) 1))
