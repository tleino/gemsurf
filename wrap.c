#include "wrap.h"

#include <stdio.h>
#include <ctype.h>
#include <string.h>

static char *wrap(const char *, int, int);

void
print_wrapped_with_leading(FILE *fp, const char *leading, char *text, int width)
{
	size_t len;
	char *wrapped;

	len = strlen(leading);
	fprintf(fp, "%s", leading);
	wrapped = wrap(text, width, len);
	fprintf(fp, "%s", wrapped);
}

void
print_wrapped(FILE *fp, char *text, int width)
{
	print_wrapped_with_leading(fp, "", text, width);
}

static char *
wrap(const char *s, int width, int indent)
{
	char out[512];
	static char fout[4096];
	int len, j, line;
	size_t i;
	const char *good_cut, *p, *begin;
	static char *nullstr = "(null)";

	if (s == NULL)
		s = nullstr;

	i = 0;
	len = 0;
	line = 0;
	begin = s;

	fout[0] = '\0';
	good_cut = begin;

	/*
	 * Iterate whole string.
	 */
	for (p = s; *p != '\0'; p++) {
		/*
		 * Remember last good wrap positions.
		 */
		if (*p == '\t' || *p == ' ' || *(p + 1) == '\0' ||
		    ispunct(*p)) {
			good_cut = p;
		}
		/*
		 * Wrap if
		 *   length limit exceeded;
		 *   end of string.
		 */
		if (len++ == width || *(p + 1) == '\0') {
			if (line > 0) {
				for (j = 0; j < indent; j++)
					out[i++] = ' ';
			}

			if (good_cut == begin)
				good_cut = p;

			p = begin;
			do {
				if (i >= sizeof(out) - 2)
					break;

				if (*p == '\t')
					out[i++] = ' ';
				else
					out[i++] = *p;
				out[i] = '\0';
			} while (p++ != good_cut);

			if (i >= sizeof(out) - 4)
				break;
			out[i++] = '\n';

			out[i] = '\0';

#ifdef __OpenBSD__
			strlcat(fout, out, sizeof(fout));
#else
			snprintf(fout, sizeof(fout), "%s", out);
#endif
			line++;

			/*
			 * Begin on a next sentence.
			 */
			i = 0;
			out[i] = '\0';

			/*
			 * Begin on a next line.
			 */
			p = good_cut;
			begin = p + 1;
			good_cut = begin;
			len = 0;
		}
	}
	return fout;
}

#ifdef TEST
int main()
{
	char *str =
	    "The strtok() and strtok_r() functions return a pointer "
	    "to the beginning of each subsequent token in the string, "
	    "after replacing the separator character itself with a NUL "
	    "character. When no more tokens remain, a null pointer is "
	    "returned. That is, tokens[0] will point to \"cat\", "
	    "tokens[1] will point to \"dog\", tokens[2] will point "
	    "to \"horse\", and tokens[3] will point to \"cow\". But "
	    "remember, the following will construct an array of pointers "
	    "to each individual word in the string s: which is something "
	    "stupid.";
	char *wrapped;

	printf("  * ");
	wrapped = wrap(str, 40, 4);
	printf("%s", wrapped);
}
#endif
