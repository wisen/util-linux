/*
 * Copyright (C) 2010-2014 Karel Zak <kzak@redhat.com>
 *
 * This file may be redistributed under the terms of the
 * GNU Lesser General Public License.
 */
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <getopt.h>

#include "c.h"
#include "nls.h"
#include "strutils.h"
#include "xalloc.h"

#include "libsmartcols.h"


enum { COL_NAME, COL_DATA };

/* add columns to the @tb */
static void setup_columns(struct libscols_table *tb)
{
	if (!scols_table_new_column(tb, "NAME", 0, SCOLS_FL_TREE))
		goto fail;
	if (!scols_table_new_column(tb, "DATA", 0, 0))
		goto fail;
	return;
fail:
	scols_unref_table(tb);
	err(EXIT_FAILURE, "failed to create output columns");
}

static struct libscols_line *add_line(struct libscols_table *tb, struct libscols_line *parent, const char *name, const char *data)
{
	struct libscols_line *ln = scols_table_new_line(tb, parent);
	if (!ln)
		err(EXIT_FAILURE, "failed to create output line");

	if (scols_line_set_data(ln, COL_NAME, name))
		goto fail;
	if (scols_line_set_data(ln, COL_DATA, data))
		goto fail;
	return ln;
fail:
	scols_unref_table(tb);
	err(EXIT_FAILURE, "failed to create output line");
}

int main(int argc, char *argv[])
{
	struct libscols_table *tb;
	struct libscols_line *ln;	/* any line */
	struct libscols_line *g1, *g2;	/* groups */
	struct libscols_line *p1, *p2;	/* parents */
	int c;

	static const struct option longopts[] = {
		{ "maxout", 0, NULL, 'm' },
		{ "width",  1, NULL, 'w' },
		{ "help",   1, NULL, 'h' },

		{ NULL, 0, NULL, 0 },
	};

	setlocale(LC_ALL, "");	/* just to have enable UTF8 chars */

	scols_init_debug(0);

	tb = scols_new_table();
	if (!tb)
		err(EXIT_FAILURE, "failed to create output table");

	while((c = getopt_long(argc, argv, "hmw:", longopts, NULL)) != -1) {
		switch(c) {
		case 'h':
			printf("%s [--help | --maxout | --width <num>]\n", program_invocation_short_name);
			break;
		case 'm':
			scols_table_enable_maxout(tb, TRUE);
			break;
		case 'w':
			scols_table_set_termforce(tb, SCOLS_TERMFORCE_ALWAYS);
			scols_table_set_termwidth(tb, strtou32_or_err(optarg, "failed to parse terminal width"));
			break;
		}
	}

	scols_table_enable_colors(tb, isatty(STDOUT_FILENO));
	setup_columns(tb);

	g1 = add_line(tb, NULL, "G1-A", "bla bla bla");

	/* standard tree (independent on grouping) */
	p1 = add_line(tb, NULL, "X-parent", "bla bla bla");
	add_line(tb, p1, "X-a", "bla bla bla");
	p2 = add_line(tb, p1, "X-b", "bla bla bla");
	add_line(tb, p2, "X-b-a", "bla bla bla");
	add_line(tb, p1, "X-c", "bla bla bla");

	/* create group G1 (from line g1 and ln) */
	ln = add_line(tb, NULL, "G1-B", "alb alb alb");
	scols_table_group_lines(tb, g1, ln);

	/* create another group G2 */
	g2 = add_line(tb, NULL, "G2-A", "bla bla bla");
	scols_table_group_lines(tb, g2, NULL);

	/* add to group g2 */
	ln = add_line(tb, NULL, "G2-B", "bla bla bla");
	scols_table_group_lines(tb, g2, ln);


	/* add member to the g1 */
	ln = add_line(tb, NULL, "G1-C", "alb alb alb");
	scols_table_group_lines(tb, g1, ln);


	/* add children to the g1 */
	ln = add_line(tb, NULL, "g1-child-A", "alb alb alb");
	scols_line_link_group(ln, g1);
	ln = add_line(tb, NULL, "g1-child-B", "alb alb alb");
	scols_line_link_group(ln, g1);

	/* add child to group g2 */
	ln = add_line(tb, NULL, "g2-child-A", "alb alb alb");
	scols_line_link_group(ln, g2);

	/* it's possible to use standard tree for group children */
	add_line(tb, ln, "g1-child-B-A", "alb alb alb");
	add_line(tb, ln, "g1-child-B-B", "alb alb alb");

	/* standard (independent) line */
	ln = add_line(tb, NULL, "foo", "bla bla bla");
	add_line(tb, ln, "foo-child", "bla bla bla");
	add_line(tb, NULL, "bar", "bla bla bla");

	scols_print_table(tb);

	scols_unref_table(tb);
	return EXIT_SUCCESS;
}