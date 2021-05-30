#include "url.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

static void parse_hostport(struct url *, const char *, char *);
static const char *default_port(const char *);
static void parse_pathfile(struct url *, char *);

int
url_parse(struct url *url, char *str)
{
	char *begin, *end;
	char *hostport;
	char *pathfile;

	begin = str;
	if ((end = strstr(begin, "://")) == NULL)
		return -1;

	*end = '\0';
	url->scheme = begin;

	begin = end + strlen("://");
	if ((end = strchr(begin, '/')) != NULL)
		*end++ = '\0';
	else
		end = "";
	hostport = begin;
	parse_hostport(url, url->scheme, hostport);

	pathfile = end;
	parse_pathfile(url, pathfile);

	return 0;
}

const char *
url_str(const struct url *url, const char *rlink)
{
	static char str[1024];
	char host[1024], path[1024];
	const char *file;

	if (default_port(url->scheme) != url->port)
		snprintf(host, sizeof(host), "%s:%s", url->host, url->port);
	else
		snprintf(host, sizeof(host), "%s", url->host);

	if (rlink != NULL && strcmp(rlink, ".") == 0)
		rlink = NULL;

	if (rlink != NULL)
		file = rlink;
	else
		file = url->file;

	if (rlink != NULL && rlink[0] == '/')
		snprintf(path, sizeof(path), "%s", ++rlink);
	else if (rlink != NULL && strcmp(rlink, "./") == 0)
		snprintf(path, sizeof(path), "%s",
		    (url->path != NULL) ? url->path : "");
	else if (url->path != NULL)
		snprintf(path, sizeof(path), "%s/%s", url->path, file);
	else
		snprintf(path, sizeof(path), "%s", file);

	snprintf(str, sizeof(str), "%s://%s/%s", url->scheme, host, path);
	return str;
}

static void
parse_hostport(struct url *url, const char *scheme, char *hostport)
{
	char *end;

	if ((end = strrchr(hostport, ':')) != NULL) {
		*end = '\0';
		url->port = ++end;
#ifdef __OpenBSD__
		if (strtonum(url->port, 1, USHRT_MAX, NULL) == 0)
			url->port = default_port(scheme);
#else
		if (atoi(url->port) == 0)
			url->port = default_port(scheme);
#endif

	} else {
		url->port = default_port(scheme);
	}
	url->host = hostport;
}

static const char *
default_port(const char *scheme)
{
	if (strcmp(scheme, "gemini") == 0)
		return "1965";
	else if (strcmp(scheme, "gopher") == 0)
		return "70";
	else if (strcmp(scheme, "https") == 0)
		return "443";
	else if (strcmp(scheme, "http") == 0)
		return "80";
	else
		return "1965";
}

static void
parse_pathfile(struct url *url, char *pathfile)
{
	char *begin, *end;

	begin = pathfile;
	if ((end = strrchr(begin, '/')) != NULL) {
		*end++ = '\0';
		url->path = begin;
		url->file = end;
	} else {
		url->path = NULL;
		url->file = begin;
	}
}

#ifndef NDEBUG
void
url_print(const struct url *url)
{
	if (url->scheme == NULL) {
		printf("(empty url)\n");
		return;
	}

	printf("scheme\t%s\n" "host\t%s\n" "port\t%s\n" "path\t%s\n"
	    "file\t%s\n", url->scheme, url->host, url->port,
	    url->path == NULL ? "(no path)" : url->path,
	    url->file);
}
#endif

#ifdef TEST
int
main(int argc, char *argv[])
{
	struct url url, base;

	printf("---- %s\n", argv[1]);
	url_parse(&url, argv[1]);
	base = url;
	url_print(&url);
	printf("%s\n", url_str(&url, NULL));

	printf("---- /rootfile\n");
	url_parse(&url, strdup(url_str(&base, "/rootfile")));
	url_print(&url);
	printf("%s\n", url_str(&url, NULL));

	printf("---- /rootfolder/\n");
	url_parse(&url, strdup(url_str(&base, "/rootfolder/")));
	url_print(&url);
	printf("%s\n", url_str(&url, NULL));

	printf("---- relativefile\n");
	url_parse(&url, strdup(url_str(&base, "relativefile")));
	url_print(&url);
	printf("%s\n", url_str(&url, NULL));

	printf("---- relativefolder/\n");
	url_parse(&url, strdup(url_str(&base, "relativefolder/")));
	url_print(&url);
	printf("%s\n", url_str(&url, NULL));

	printf("---- relativefolder/file\n");
	url_parse(&url, strdup(url_str(&base, "relativefolder/file")));
	url_print(&url);
	printf("%s\n", url_str(&url, NULL));

	printf("---- relativefolder/folder2/file\n");
	url_parse(&url, strdup(url_str(&base, "relativefolder/folder2/file")));
	url_print(&url);
	printf("%s\n", url_str(&url, NULL));

	printf("---- ./\n");
	base = url;
	url_parse(&url, strdup(url_str(&base, "./")));
	url_print(&url);
	printf("%s\n", url_str(&url, NULL));

	printf("---- .\n");
	url_parse(&url, strdup(url_str(&base, ".")));
	url_print(&url);
	printf("%s\n", url_str(&url, NULL));

	return 0;
}
#endif
