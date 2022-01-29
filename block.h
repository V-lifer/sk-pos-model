#ifndef BLOCK_H
#define BLOCK_H

int blocking_init(struct context *c, const char *file);

void blocking_free(struct context *c);

int is_blocked(struct context *