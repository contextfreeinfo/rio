#pragma once

struct rio_Sys;

typedef struct rio_Sys {
    void* state;
    void (*log)(const struct rio_Sys*, const char* message);
} rio_Sys;

void rio_log(const rio_Sys*, const char* message);
