#include "sys.h"

void rio_log(const rio_Sys* self, const char* message) {
    self->log(self, message);
}
