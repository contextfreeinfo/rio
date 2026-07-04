#include <stdlib.h>
#include <stdio.h>
#include "engine.h"

void rio_main_System_log_puts(const rio_System*, const char* message) {
    puts(message);
}

int main() {
    rio_System system = {
        .state = NULL,
        .log = rio_main_System_log_puts,
    };
    rio_log(&system, "Hi there!");
    rio_log(&system, "Bye y'all!");
    return 0;
}
