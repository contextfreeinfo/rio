(module
  (type (;0;) (func (param i32) (result i32)))
  (func (;0;) (type 0) (param i32) (result i32)
    (local i32 i32 i32)
    global.get 0
    i32.const 32
    i32.sub
    local.tee 1
    global.set 0
    block  ;; label = @1
      local.get 0
      i32.const 0
      i32.le_s
      if  ;; label = @2
        local.get 1
        i32.const 0
        i32.store offset=4
        br 1 (;@1;)
      end
      block  ;; label = @2
        local.get 0
        i32.const 1
        i32.eq
        if  ;; label = @3
          local.get 1
          i32.const 1
          i32.store offset=4
          br 1 (;@2;)
        end
        local.get 1
        local.get 0
        i32.const 2
        i32.sub
        local.tee 2
        i32.store offset=8
        local.get 1
        local.get 0
        local.get 2
        i32.le_s
        i32.store8 offset=12
        local.get 1
        i32.load8_u offset=12
        i32.const 1
        i32.and
        if  ;; label = @3
          unreachable
        end
        local.get 1
        i32.load offset=8
        call 0
        local.set 2
        local.get 1
        local.get 0
        i32.const 1
        i32.sub
        local.tee 3
        i32.store offset=16
        local.get 1
        local.get 0
        local.get 3
        i32.le_s
        i32.store8 offset=20
        local.get 1
        i32.load8_u offset=20
        i32.const 1
        i32.and
        if  ;; label = @3
          unreachable
        end
        local.get 1
        local.get 1
        i32.load offset=16
        call 0
        local.tee 0
        local.get 2
        i32.add
        local.tee 3
        i32.store offset=24
        local.get 1
        local.get 0
        i32.const 0
        i32.lt_s
        local.get 2
        local.get 3
        i32.gt_s
        i32.xor
        i32.store8 offset=28
        local.get 1
        i32.load8_u offset=28
        i32.const 1
        i32.and
        if  ;; label = @3
          unreachable
        end
        local.get 1
        local.get 1
        i32.load offset=24
        i32.store offset=4
      end
    end
    local.get 1
    i32.load offset=4
    local.set 0
    local.get 1
    i32.const 32
    i32.add
    global.set 0
    local.get 0)
  (memory (;0;) 17)
  (global (;0;) (mut i32) (i32.const 1048576))
  (export "memory" (memory 0))
  (export "fib" (func 0))
  (data (;0;) (i32.const 1048576) "integer overflow\00\00\00\00\00\00\00\00integer overflow\00\00\00\00\00\00\00\00integer overflow"))
