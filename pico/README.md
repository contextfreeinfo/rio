```sh
time cmake -S . -B build-host -DPROJECT_TYPE=HOST
time cmake --build build-host --target hi && ls -l build-host
```

or

```sh
time cmake -S . -B build-pico -DPROJECT_TYPE=PICO
time cmake --build build-pico --target hi && ls -l build-pico
```
