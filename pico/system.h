#pragma once

struct rio_System;

typedef struct rio_System {
    void* state;
    void (*log)(const struct rio_System*, const char* message);
} rio_System;

void rio_log(const rio_System*, const char* message);
