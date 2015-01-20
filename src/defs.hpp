#include <cstdarg>

__attribute__((noreturn))
void diev(const char *msg, va_list ap);

__attribute__((noreturn))
void die(const char *msg, ...);
