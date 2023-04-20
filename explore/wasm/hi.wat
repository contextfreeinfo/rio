;; Originally from:
;; https://github.com/danbev/learning-wasi/blob/master/src/fd_write.wat
(module
  (import "wasi_snapshot_preview1" "fd_write" 
    (func $print
      (param $fd i32) 
      (param $iovec i32)
      (param $len i32)
      (param $written i32)
      (result i32)))

  (memory 1)
  (export "memory" (memory 0))

  ;; 8 is the offset to write to
  (data (i32.const 8) "Hi there!\n")

  (func $main (export "_start")
    call $talk
    call $talk
  )

  (func $talk
    ;; Build iovec_t instance
    ;; load offset and string start value
    i32.const 0
    i32.const 8
    i32.store
    ;; load offset and string length value
    i32.const 4
    i32.const 10
    i32.store
    ;; Params for fd_write
    i32.const 1  ;; 1 for stdout
    i32.const 0  ;; 0 as we stored the beginning of __wasi_ciovec_t
    i32.const 1  ;; number of iovec instances
    i32.const 20 ;; nwritten address
    call $print
    ;; Discard error code
    drop
  )
)
