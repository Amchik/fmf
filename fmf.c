#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __STDC_VERSION__
#if __STDC_VERSION__ < 199901L
typedef signed long ssize_t;
#endif
#else
#define ssize_t signed long
#endif

enum State {
	S_NOTHING,
	S_IN_P,
	S_IN_CODE, /* here: codeblock, ```, <pre> */
	S_IN_UL,
	S_IN_OL
};
#define F_LI_WAIT_CLOSE 1

char
get_line_type(const char *line, ssize_t nread, size_t *off) {
	ssize_t i, h;

	for (i = 0; (line[i] == ' ' || line[i] == '\t') && i < nread; ++i);
	if (line[i] == '\n' || i == nread)
		return(' '); /* empty */

	if (i + 3 < nread && line[i] == '`' && line[i + 1] == '`' && line[i + 2] == '`') {
		*off = i + 3;
		return('`'); /* code block */
	}

	if (line[i] == '#') {
		for (h = 0; line[i + h] == '#'; ++h);
		*off = i + h;
		return(h); /* heading */
	}

	if (line[i] == '-' || line[i] == '@') {
		*off = i + 1;
		return(line[i]);
	}

	return(0); /* normal */
}

char *
parse_code_meta(const char *line, size_t offset, ssize_t nread) {
	size_t offset2;
	char *str;

	str = malloc(nread - offset + 14);
	str[0] = 0;

	for (offset2 = offset; line[offset2] != ',' && offset2 < nread; ++offset2);
	str[0] = 'l';
	str[1] = 'a';
	str[2] = 'n';
	str[3] = 'g';
	str[4] = ':';
	str[5] = ' ';
	memcpy(str + 6, line + offset, offset2 - offset);

	if (offset2 == nread) {
		str[6 + offset2 - offset] = 0;
		return(str);
	}
	str[offset2 - offset + 6] = ',';
	str[offset2 - offset + 7] = ' ';
	str[offset2 - offset + 8] = 'f';
	str[offset2 - offset + 9] = 'i';
	str[offset2 - offset +10] = 'l';
	str[offset2 - offset +11] = 'e';
	str[offset2 - offset +12] = ':';
	str[offset2 - offset +13] = ' ';
	memcpy(str + offset2 - offset + 14, line + offset2 + 1, nread - offset2 - 2);
	str[nread - offset + 12] = 0;

	return(str);
}

void
print_line(const char *line, size_t nread, size_t off, int incode) {
	size_t i, j;
	char esc, eesc, *buff, *buff2;

	i = off;
	while (i < nread) {
        if (incode)
            for (j = i; j < nread && line[j] != '&' && line[j] != '<' && line[j] != '>' && line[j] != '\\'; ++j);
        else
            for (j = i; j < nread && line[j] != '&' && line[j] != '<' && line[j] != '>' && line[j] != '*' && line[j] != '_' && line[j] != '`' && line[j] != '\\'; ++j);
		if (j > i) {
write:
			fwrite(line + i, 1, j - i, stdout);
		}
		if (j >= nread) break;
        if (incode) {
            esc = line[j];
            i = ++j;
            if      (esc == '&') { printf("&amp;"); continue; }
            else if (esc == '<') { printf("&lt;");  continue; }
            else if (esc == '>') { printf("&gt;");  continue; }
        }
        /* todo: macro */
        if (incode && line[j] != '&' && line[j] != '<' && line[j] != '>' && line[j] != '*' && line[j] != '_' && line[j] != '`' && line[j] != '\\') {
            fwrite(line + i - 2, 1, 2, stdout);
            continue;
        }
		esc = line[j];
		eesc = esc;
		i = j + 1;
		if (esc == '\\') {
			j += 1;
			i += 1;
			esc = line[j];
			if (esc == '(') eesc = ')';
			else if (esc == '[') eesc = ']';
			else {
				putchar(esc);
				continue;
			}
		}
		else if (esc == '&') { printf("&amp;"); continue; }
		else if (esc == '<') { printf("&lt;");  continue; }
		else if (esc == '>') { printf("&gt;");  continue; }

		for (j += 1; j < nread && line[j] != eesc; ++j);
		if (j == nread)
			goto write;
		buff = malloc(j - i + 1);
		memcpy(buff, line + i, j - i);
		buff[j - i] = 0;

		if (esc == '*')      printf("<b>%s</b>", buff);
		else if (esc == '_') printf("<i>%s</i>", buff);
		else if (esc == '`') printf("<code>%s</code>", buff);
		else if (esc == '(') printf("<a href=\"https://%s\">%s</a>", buff, buff);
		else if (line[j + 1] != '(') printf("<a>%s</a>", buff);
		else {
			i = j + 2;
			for (j += 2; j < nread && line[j] != ')'; ++j);
			if (j == nread) {
				free(buff);
				goto write;
			}
			buff2 = malloc(j - i + 1);
			memcpy(buff2, line + i, j - i);
			buff2[j - i] = 0;
			printf("<a href=\"https://%s\">%s</a>", buff2, buff);
			free(buff2);
		}

		free(buff);
		i = j + 1;
	}
}

int
main(int argc, char **argv) {
	FILE *ftmp, *fpage;
	enum State state;
	int flags;
	char *line, *buff, lty;
	size_t linen, off;
	ssize_t nread;

	if (argc != 2) {
		puts("Usage: build.c <page>");
		return(1);
	}
	fpage = fopen(argv[1], "r");
	if (!fpage) {
		perror("Failed to open page");
		return(1);
	}

	linen = 0;
	line = 0;
	flags = 0;
	state = S_NOTHING;
	while ((nread = getline(&line, &linen, fpage)) > 0) {
		lty = get_line_type(line, nread, &off);
		if (lty == ' ' && state != S_IN_CODE) {
			if (state == S_IN_P) {
				state = S_NOTHING;
				puts("</p>");
			} else if (state == S_IN_UL) {
				state = S_NOTHING;
				puts("</ul>");
			} else if (state == S_IN_OL) {
				state = S_NOTHING;
				puts("</ol>");
			}
			continue;
		} else if (lty == '`') {
			if (state == S_IN_CODE) {
				puts("</pre>");
				state = S_NOTHING;
			} else if (state == S_NOTHING) {
				state = S_IN_CODE;
				buff = parse_code_meta(line, off, nread);
				printf("<div class=\"codeblock\">\n"
						"<div class=\"prelude\">%s</div>\n"
						"<pre>", buff);
				free(buff);
			} else {
				puts("    ```");
			}
			continue;
		} else if ((state == S_NOTHING || state == S_IN_UL) && lty == '-') {
			line[--nread] = 0; /* maybe ub, idk */
			if (state == S_NOTHING) {
				puts("<ul>");
				state = S_IN_UL;
			}
			if (flags & F_LI_WAIT_CLOSE) {
				flags &= ~F_LI_WAIT_CLOSE;
				puts("</li>");
			}
			printf("<li>");
			print_line(line, nread, off, 0);
			flags |= F_LI_WAIT_CLOSE;
			continue;
		} else if ((state == S_NOTHING || state == S_IN_OL) && lty == '@') {
			line[--nread] = 0; /* maybe ub, idk */
			if (state == S_NOTHING) {
				puts("<ol>");
				state = S_IN_OL;
			}
			if (flags & F_LI_WAIT_CLOSE) {
				flags &= ~F_LI_WAIT_CLOSE;
				puts("</li>");
			}
			printf("<li>");
			print_line(line, nread, off, 0);
			flags |= F_LI_WAIT_CLOSE;
			continue;
		} else if (lty > 0 && state == S_NOTHING) {
			line[nread - 1] = 0; /* maybe ub, idk */
			printf("<h%d>%s</h%d>\n", lty, line + off, lty);
			continue;
		}

		if (state == S_NOTHING) {
			state = S_IN_P;
			puts("<p>");
		}

        print_line(line, nread, 0, state == S_IN_CODE);
	}
	if (state == S_IN_P)
		puts("</p>");
	else if (flags & F_LI_WAIT_CLOSE)
		puts("</li>");

    return(0);
}
