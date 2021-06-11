#include "fetch.h"
#include "viewbuf.h"
#include "gemtext.h"
#include "url.h"
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>

void
usage(const char *prog)
{
	fprintf(stderr, "Usage: %s [-n] [-p prompt] [url]\n", prog);
	exit(1);
}

struct page
{
	int linenumbers;
	FILE *fp;
	int lines;
	struct viewbuf *vb;
};

static void
linecb(const char *s, void *data)
{
	struct page *page = (struct page *) data;
	FILE *fp;
	char *buf = NULL;
	size_t sz = 0;
	char *begin, *p;
	char *tmp;

	fp = open_memstream(&buf, &sz);
	tmp = strdup(s);
	gemtext_puts(tmp, fp);
	fclose(fp);
	free(tmp);

	begin = p = buf;
	while ((p = strchr(p, '\n')) != NULL) {
		*p++ = '\0';
		viewbuf_add(page->vb, begin, s);

		if (page->linenumbers)
			fprintf(page->fp, "%d\t%s\n", VIEWBUF_NLINES(page->vb),
			    begin);
		else
			fprintf(page->fp, "%s\n", begin);

		begin = p;
	}
	free(buf);
}

int
main(int argc, char *argv[])
{
	int ch;
	int linenumbers = 0;
	char buf[256], *p;
	char *urlstr;
	struct page page = { 0 };
	int lno;
	char *src, *srcp, *link, *tmp;
	char *prompt = NULL;
	struct url url;
	int running;

	while ((ch = getopt(argc, argv, "np:")) != -1) {
		switch (ch) {
		case 'p':
			prompt = optarg;
			break;
		case 'n':
			linenumbers ^= 1;
			break;
		default:
			usage(argv[0]);
		}
	}
	argc -= optind;
	argv += optind;

	if (argc) {
		urlstr = strdup(*argv);
		if (urlstr == NULL)
			err(1, "strdup");
	} else
		urlstr = NULL;

	page.fp = stdout;
	page.linenumbers = linenumbers;
	page.vb = viewbuf_create();

	if (urlstr != NULL)
		fetch(urlstr, linecb, &page);

	if (prompt != NULL)
		printf("%s", prompt);
	fflush(stdout);

	running = 1;
	while (running && fgets(buf, sizeof(buf), stdin) != NULL) {
		p = buf;
		buf[strcspn(buf, "\r\n")] = '\0';

		switch (*p) {
		case 'b':
			p++;
			while (isspace(*p))
				p++;

			if (urlstr != NULL)
				free(urlstr);
			urlstr = strdup(p);
			if (urlstr == NULL)
				err(1, "strdup");

			if (page.vb != NULL)
				viewbuf_free(page.vb);
			page.fp = stdout;
			page.linenumbers = linenumbers;
			page.vb = viewbuf_create();
			fetch(urlstr, linecb, &page);
			break;
		case 'x':
			p++;
			while (isspace(*p))
				p++;
			lno = atoi(p);
			if (lno > 0) {
				src = VIEWBUF_SRC(page.vb, lno - 1);
				if (src[0] != '=' || src[1] != '>')
					break;
				srcp = strdup(&src[2]);
				if (srcp == NULL)
					err(1, "strdup");
				while (*srcp != '\0' && isspace(*srcp))
					srcp++;
				link = srcp;
				while (*srcp != '\0' && !isspace(*srcp))
					srcp++;
				*srcp = '\0';

				if (strstr(link, "gemini://") == NULL) {
					url_parse(&url, urlstr);
					tmp = strdup(url_str(&url, link));
					if (tmp == NULL)
						err(1, "strdup");
					free(urlstr);
					urlstr = tmp;
				} else {
					free(urlstr);
					urlstr = strdup(link);
					if (urlstr == NULL)
						err(1, "strdup");
				}

				if (page.vb != NULL)
					viewbuf_free(page.vb);
				page.fp = stdout;
				page.linenumbers = linenumbers;
				page.vb = viewbuf_create();
				fetch(urlstr, linecb, &page);
			}
			break;
		case 'l':
			p++;
			while (isspace(*p))
				p++;
			if (isdigit(*p)) {
				lno = atoi(p);
				if (lno > 0) {
					puts(VIEWBUF_SRC(page.vb, lno - 1));
				}
			}
			break;
		case 'q':
			running = 0;
			break;
		}

		if (prompt != NULL)
			printf("%s", prompt);
		fflush(stdout);
	}

	if (urlstr != NULL)
		free(urlstr);

	if (page.vb != NULL)
		viewbuf_free(page.vb);

	return 0;
}
