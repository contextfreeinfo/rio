#include "lex.h"

rio_Err rio_lex(rio_File file) {
    char c;
    while (!rio_read(file, &c)) {
        char message[2] = {c, 0};
        rio_log(message);
    }
    return 0;
}
