/*	$NetBSD: scroll.c,v 1.11 2000/04/11 13:57:10 blymn Exp $	*/

/*
 * Copyright (c) 1981, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)scroll.c	8.3 (Berkeley) 5/4/94";
#else
__RCSID("$NetBSD: scroll.c,v 1.11 2000/04/11 13:57:10 blymn Exp $");
#endif
#endif				/* not lint */

#include "curses.h"
#include "curses_private.h"

/*
 * scroll --
 *	Scroll the window up a line.
 */
int
scroll(win)
	WINDOW *win;
{
	return(wscrl(win, 1));
}

/*
 * wscrl --
 *	Scroll a window n lines - up if n is positive, down if n is negative.
 */
int
wscrl(win, lines)
	WINDOW *win;
	int	lines;
{
	int     oy, ox;

#ifdef DEBUG
	__CTRACE("wscrl: (%0.2o) lines=%d\n", win, lines);
#endif

	if (!(win->flags & __SCROLLOK))
		return (ERR);
	if (!lines)
		return (OK);

	getyx(win, oy, ox);
	wmove(win, 0, 0);
	winsdelln(win, 0 - lines);
	wmove(win, oy, ox);

	if (win == curscr) {
		putchar('\n');
		if (!NONL)
			win->curx = 0;
#ifdef DEBUG
		__CTRACE("scroll: win == curscr\n");
#endif
	}
	return (OK);
}
