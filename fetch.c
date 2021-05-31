#include "fetch.h"
#include "url.h"
#include "linebuf.h"

#include <tls.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <err.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>

static void write_line(int fd, const char *);

struct session
{
	int fd;
	struct linebuf		*lb;
	char			*firstline;
};

struct session *
fetch_open(const struct url *url, int *code, char **firstline)
{
	struct sockaddr_in sa = { 0 };
	char *s, *p;
	struct session *sess;

	sess = calloc(1, sizeof(struct session));
	if (sess == NULL)
		return NULL;

	sess->fd = socket(AF_INET, SOCK_STREAM, 0);
	if (sess->fd < 0)
		err(1, "socket");

	sa.sin_family = AF_INET;
	sa.sin_port = htons(1965);
	sa.sin_addr.s_addr = inet_addr("127.0.0.1");

	if (connect(sess->fd, (struct sockaddr *) &sa, sizeof(sa)) < 0)
		err(1, "connect");

	write_line(sess->fd, url_str(url, NULL));

	sess->lb = linebuf_create();

	s = NULL;
	while (linebuf_fill_from_fd(sess->lb, sess->fd) > 0)
		if ((s = linebuf_read(sess->lb)) != NULL)
			break;
	if (s == NULL) {
		warnx("couldn't read first line");
		return NULL;
	}
	p = strchr(s, ' ');
	if (p != NULL) {
		*p++ = '\0';
		sess->firstline = strdup(p);
		*code = atoi(s);
	}
	*firstline = sess->firstline;

	return sess;
}

void
fetch_close(struct session *sess)
{
	close(sess->fd);
	if (sess->firstline != NULL)
		free(sess->firstline);

	free(sess);
}

void
fetch_linestream(struct session *sess,
    void (*linecb)(const char *line, void *data), void *data)
{
	char *s;

	while (linebuf_fill_from_fd(sess->lb, sess->fd) > 0)
		while ((s = linebuf_read(sess->lb)) != NULL)
			linecb(s, data);

	linebuf_free(sess->lb);
}

int
fetch(const char *s, void (*linecb)(const char *line, void *data), void *data)
{	
	struct session *sess;
	int status;
	char *meta;
	struct url url;
	char *ustr;
	char *p;
	int ret;

	ustr = strdup(s);
	if (ustr == NULL) {
		err(1, "strdup");
		return -1;
	}

	if (url_parse(&url, ustr) == -1) {
		errx(1, "error parsing url");
		return -1;
	}
	if (strcmp(url.scheme, "gemini") != 0) {
		errx(1, "only 'gemini' scheme is supported");
		return -1;
	}
	sess = fetch_open(&url, &status, &meta);
	if (sess == NULL) {
		errx(1, "couldn't connect caching server");
		return -1;
	}

	if (status >= 10 && status <= 19) {
		errx(1, "requires input");
	} else if (status >= 30 && status <= 39) {
		printf("redirect %s\n", meta);
		p = strdup(meta);
		fetch_close(sess);
		ret = fetch(p, linecb, data);
		free(p);
		return ret;
	} else if (status >= 40 && status <= 49) {
		errx(1, "temp failure: %s", meta);
	} else if (status >= 50 && status <= 59) {
		errx(1, "permanent failure: %s", meta);
	} else if (status >= 60 && status <= 69) {
		errx(1, "client certificate required");
	} else if (status < 20 || status > 29) {
		errx(1, "unsupported status: %d", status);
	}

	if (!(strncmp(meta, "text/gemini", strlen("text/gemini")) == 0 ||
	    strncmp(meta, "text/plain", strlen("text/plain")) == 0)) {
		errx(1, "unsupported content type");
		return -1;
	}

	fetch_linestream(sess, linecb, data);
	fetch_close(sess);
	return 0;
}

static void
write_line(int fd, const char *str)
{
	size_t len, newlen;
	ssize_t ret;
	const char *p;
	char *s;

	len = strlen(str);
	newlen = len + strlen("\r\n");
	if ((s = malloc(newlen + 1)) == NULL)
		err(1, "malloc");

#ifdef __OpenBSD__
	strlcpy(s, str, newlen);
#else
	snprintf(s, newlen, "%s", str);
#endif
	s[newlen-2] = '\r';
	s[newlen-1] = '\n';
	s[newlen] = '\0';

	len = newlen;
	p = s;
	while (len > 0) {
		ret = write(fd, p, len);
		if (ret == -1)
			err(1, "write");
		p += ret;
		len -= ret;
	}

	free(s);
}
