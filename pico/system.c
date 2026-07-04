#include "system.h"

void rio_log(const rio_System* self, const char* message) {
    self->log(self, message);
}
