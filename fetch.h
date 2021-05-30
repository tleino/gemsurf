#ifndef FETCH_H
#define FETCH_H

#include <stdio.h>

struct session;
struct url;

struct session			*fetch_open(const struct url *,
				    int *, char **);
void				 fetch_close(struct session *);
void				 fetch_linestream(struct session *,
				    void (*)(const char *, void *), void *);
int				 fetch(const char *,
				    void (*)(const char *, void *), void *);

#endif
