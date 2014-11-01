#ifndef OGL_UTILS_H
#define OGL_UTILS_H 1

#include <boolean.h>

boolean
ogl_check_errors (char *msg);

boolean
ogl_display_mode_not_available (void);

void
ogl_display_mode_not_available_fatal (void);

#endif
