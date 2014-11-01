#ifndef ARGS_H
#define ARGS_H 1

extern int args_number;
extern char *args_command;

void
args_initialize (int argc, char *argv[]);

void
args_flag (int number);

int
args_unflagged (void);

int
args_exists (char *arg);

char *
args_item (int number);

#endif
