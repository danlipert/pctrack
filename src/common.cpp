#include "defs.hpp"
#include <cstdio>
#include <cstdlib>

__attribute__((noreturn))
void diev(const char *msg, va_list ap) {
    std::fputs("Error: ", stderr);
    std::vfprintf(stderr, msg, ap);
    std::fputc('\n', stderr);
    std::exit(1);
}

__attribute__((noreturn))
void die(const char *msg, ...) {
    va_list ap;
    va_start(ap, msg);
    diev(msg, ap);
}
