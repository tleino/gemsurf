#ifndef WRAP_H
#define WRAP_H

#include <stdio.h>

void print_wrapped_with_leading(FILE *, const char *leading, char *text, int width);
void print_wrapped(FILE *, char *text, int width);

#endif
