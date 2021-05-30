#include "fetch.h"
#include "viewbuf.h"
#include "gemtext.h"
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

void
usage(const char *prog)
{
	fprintf(stderr, "Usage: %s [-n] [url]\n", prog);
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
	char *url;
	struct page page = { 0 };
	int lno;

	while ((ch = getopt(argc, argv, "n")) != -1) {
		switch (ch) {
		case 'n':
			linenumbers ^= 1;
			break;
		default:
			usage(argv[0]);
		}
	}
	argc -= optind;
	argv += optind;

	if (argc)
		url = *argv;
	else
		url = NULL;

	page.fp = stdout;
	page.linenumbers = linenumbers;
	page.vb = viewbuf_create();
	if (url != NULL)
		fetch(url, linecb, &page);

	while (fgets(buf, sizeof(buf), stdin) != NULL) {
		p = buf;
		buf[strcspn(buf, "\r\n")] = '\0';

		switch (*p) {
		case 'b':
			p++;
			while (isspace(*p))
				p++;
			url = p;
			if (page.vb != NULL)
				viewbuf_free(page.vb);
			page.fp = stdout;
			page.linenumbers = linenumbers;
			page.vb = viewbuf_create();
			fetch(url, linecb, &page);
			break;
		case 'x':
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
		}
	}

	if (page.vb != NULL)
		viewbuf_free(page.vb);

	return 0;
}
