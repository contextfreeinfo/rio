#include <stdlib.h>
#include <stdio.h>
#include "engine.h"

void rio_main_Sys_log_puts(const rio_Sys*, const char* message) {
    puts(message);
}

int main() {
    rio_Sys sys = {
        .state = NULL,
        .log = rio_main_Sys_log_puts,
    };
    rio_log(&sys, "Hi there!");
    rio_log(&sys, "Bye y'all!");
    return 0;
}
