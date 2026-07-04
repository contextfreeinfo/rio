#include "system.h"

void rio_System_log(rio_System* self, char* message) {
    self->log(self, message);
}
