/**
 * RC Translation - RC@IST/UL
 * Common header impl - RC45179 15'16
 *
 * @author: Rui Ventura (ist181045)
 * @author: Diogo Freitas (ist181586)
 * @author: Sara Azinhal (ist181700)
 */

#include <stdarg.h>
#include <stdio.h>

#include "rctr.h"

/* Function implementations */

int eprintf(const char *format, ...) {
  int ret;
  va_list args;

  va_start(args, format);
  ret = vfprintf(stderr, format, args);
  va_end(args);

  return ret;
}
