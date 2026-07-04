#pragma once

struct rio_System;

typedef struct rio_System {
    void* state;
    void (*log)(struct rio_System*, char* message);
} rio_System;

void rio_System_log(rio_System*, char* message);
