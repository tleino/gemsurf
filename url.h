#ifndef URL_H
#define URL_H

struct url
{
	const char *scheme;
	const char *host;
	const char *port;
	const char *path;
	const char *file;
};

/*
 * url_parse(dst, str):
 */
int url_parse(struct url *, char *);

/*
 * url_str(src, rlink):
 */
const char *url_str(const struct url *, const char *);

#ifndef NDEBUG
/*
 * url_print(src):
 */
void url_print(const struct url *);
#endif

#endif
