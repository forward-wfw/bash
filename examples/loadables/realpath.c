/*
 * realpath -- canonicalize pathnames, resolving symlinks
 *
 * usage: realpath [-csv] [-a name] pathname [pathname...]
 *
 * options:	-a name	assign each canonicalized pathname to indexed array
 *			variable NAME
 *		-c	check whether or not each resolved path exists
 *		-s	no output, exit status determines whether path is valid
 *		-v	produce verbose output
 *
 *
 * exit status:	0	if all pathnames resolved
 *		1	if any of the pathname arguments could not be resolved
 *
 *
 * Bash loadable builtin version
 *
 * Chet Ramey
 * chet@po.cwru.edu
 */

/*
   Copyright (C) 1999-2009,2021 Free Software Foundation, Inc.

   This file is part of GNU Bash.
   Bash is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Bash is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Bash.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif
#include "bashansi.h"
#include <maxpath.h>
#include <errno.h>

#include "builtins.h"
#include "shell.h"
#include "bashgetopt.h"
#include "common.h"

#ifndef errno
extern int	errno;
#endif

extern char	*sh_realpath();

int
realpath_builtin(list)
WORD_LIST	*list;
{
	int	opt, cflag, vflag, sflag, aflag, es;
	char	*r, realbuf[PATH_MAX], *p;
	struct stat sb;
#if defined (ARRAY_VARS)
	arrayind_t	ind;
	char	*aname;
	SHELL_VAR	*v;
#endif

	if (list == 0) {
		builtin_usage();
		return (EX_USAGE);
	}

	vflag = cflag = sflag = aflag = 0;
#if defined (ARRAY_VARS)
	aname = NULL;
	v = NULL;
	ind = 0;
#endif
	reset_internal_getopt();
	while ((opt = internal_getopt (list, "a:csv")) != -1) {
		switch (opt) {
#if defined (ARRAY_VARS)
		case 'a':
			aflag = 1;
			aname = list_optarg;
			break;
#endif
		case 'c':
			cflag = 1;
			break;
		case 's':
			sflag = 1;
			break;
		case 'v':
			vflag = 1;
			break;
		CASE_HELPOPT;
		default:
			builtin_usage();
			return (EX_USAGE);
		}
	}

	list = loptend;

	if (list == 0) {
		builtin_usage();
		return (EX_USAGE);
	}

#if defined (ARRAY_VARS)
	if (aflag && legal_identifier (aname) == 0) {
		sh_invalidid(aname);
		return (EXECUTION_FAILURE);
	}
	if (aname && builtin_unbind_variable (aname) == -2)
		return (EXECUTION_FAILURE);
	if (aname) {
		v = find_or_make_array_variable (aname, 1);
		if (v == 0 || readonly_p (v) || noassign_p (v)) {
			if (v && readonly_p (v))
				err_readonly (aname);
			return (EXECUTION_FAILURE);
		} else if (array_p (v) == 0) {
			builtin_error ("%s: not an indexed array", aname);
			return (EXECUTION_FAILURE);
		}
		if (invisible_p (v))
			VUNSETATTR (v, att_invisible);
		array_flush (array_cell (v));
	}
#endif

	for (es = EXECUTION_SUCCESS; list; list = list->next) {
		p = list->word->word;
		r = sh_realpath(p, realbuf);
		if (r == 0) {
			es = EXECUTION_FAILURE;
			if (sflag == 0)
				builtin_error("%s: cannot resolve: %s", p, strerror(errno));
			continue;
		}
		if (cflag && (stat(realbuf, &sb) < 0)) {
			es = EXECUTION_FAILURE;
			if (sflag == 0)
				builtin_error("%s: %s", p, strerror(errno));
			continue;
		}
		if (sflag == 0) {
			if (aflag) {
				bind_array_element (v, ind, realbuf, 0);
				ind++;
			} else {
				if (vflag)
					printf ("%s -> ", p);
				printf("%s\n", realbuf);
			}
		}
	}
	return es;
}

char *realpath_doc[] = {
	"Display pathname in canonical form.",
	"",
	"Display the canonicalized version of each PATHNAME argument, resolving",
	"symbolic links.  The -c option checks whether or not each resolved name",
	"exists.  The -s option produces no output; the exit status determines the",
	"validity of each PATHNAME.  The -v option produces verbose output.  The",
	"exit status is 0 if each PATHNAME was resolved; non-zero otherwise.",
	(char *)NULL
};

struct builtin realpath_struct = {
	"realpath",		/* builtin name */
	realpath_builtin,	/* function implementing the builtin */
	BUILTIN_ENABLED,	/* initial flags for builtin */
	realpath_doc,		/* array of long documentation strings */
	"realpath [-csv] pathname [pathname...]",	/* usage synopsis */
	0			/* reserved for internal use */
};
