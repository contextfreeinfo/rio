#include <stdlib.h>
#include <stdio.h>
#include "engine.h"

void rio_main_System_log_puts(rio_System*, char* message) {
    puts(message);
}

int main() {
    rio_System system = {
        .state = NULL,
        .log = rio_main_System_log_puts,
    };
    rio_System_log(&system, "Hi there!");
    rio_System_log(&system, "Bye y'all!");
    return 0;
}
