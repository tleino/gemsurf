#include "gemtext.h"
#include "wrap.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define LINE_WIDTH 72

void
gemtext_puts(char *text, FILE *fp)
{
	char *desc;

	if (text[0] == '\0') {
		fprintf(fp, "\n");
		return;
	}

	if (text[0] == '#') {
		print_wrapped_with_leading(fp, "#", text, LINE_WIDTH);
	}
	else if (text[0] == '*') {
		text = &text[1];
		while (*text != '\0' && isspace(*text))
			text++;
		print_wrapped_with_leading(fp, " * ", text, LINE_WIDTH);
	}
	else if (text[0] == '=' && text[1] == '>') {
		desc = &text[2];
		while (*desc != '\0' && isspace(*desc))
			desc++;
		while (*desc != '\0' && !isspace(*desc))
			desc++;
		while (*desc != '\0' && isspace(*desc))
			desc++;
		if (*desc == '\0') {
			desc = &text[2];
			while (*desc != '\0' && isspace(*desc))
				desc++;
		}
		print_wrapped_with_leading(fp, " => ", desc, LINE_WIDTH);
	} else if (text[0] == '>')
		print_wrapped_with_leading(fp, "\t",
		    &text[1], LINE_WIDTH - 6);
	else {
		print_wrapped_with_leading(fp, "", text, LINE_WIDTH);
	}
}
