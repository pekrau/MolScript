#ifndef ERR_H
#define ERR_H 1

void
err_set_prefix (const char *new);

void
err_fatal (const char *msg);

void
err_message (const char *msg);

void
err_usage (const char *msg);

void
err_warning (const char *msg);

#endif
