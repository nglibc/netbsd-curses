/*	$NetBSD: newwin.c,v 1.16 2000/04/15 13:17:04 blymn Exp $	*/

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
static char sccsid[] = "@(#)newwin.c	8.3 (Berkeley) 7/27/94";
#else
__RCSID("$NetBSD: newwin.c,v 1.16 2000/04/15 13:17:04 blymn Exp $");
#endif
#endif				/* not lint */

#include <stdlib.h>

#include "curses.h"
#include "curses_private.h"

#undef	nl			/* Don't need it here, and it interferes. */

extern struct __winlist	*winlistp;

static WINDOW *__makenew __P((int, int, int, int, int));

void __set_subwin __P((WINDOW *, WINDOW *));

/*
 * newwin --
 *	Allocate space for and set up defaults for a new window.
 */
WINDOW *
newwin(int nl, int nc, int by, int bx)
{
	WINDOW *win;
	__LINE *lp;
	int     i, j;
	__LDATA *sp;

	if (nl == 0)
		nl = LINES - by;
	if (nc == 0)
		nc = COLS - bx;

	if ((win = __makenew(nl, nc, by, bx, 0)) == NULL)
		return (NULL);

	win->nextp = win;
	win->ch_off = 0;
	win->orig = NULL;
	win->delay = -1;

#ifdef DEBUG
	__CTRACE("newwin: win->ch_off = %d\n", win->ch_off);
#endif

	for (i = 0; i < nl; i++) {
		lp = win->lines[i];
		lp->flags = 0;
		for (sp = lp->line, j = 0; j < nc; j++, sp++) {
			sp->ch = ' ';
			sp->attr = 0;
		}
		lp->hash = __hash((char *)(void *)lp->line,
		    (int) (nc * __LDATASIZE));
	}
	return (win);
}

WINDOW *
subwin(WINDOW *orig, int nl, int nc, int by, int bx)
{
	int     i;
	__LINE *lp;
	WINDOW *win;

	/* Make sure window fits inside the original one. */
#ifdef	DEBUG
	__CTRACE("subwin: (%0.2o, %d, %d, %d, %d)\n", orig, nl, nc, by, bx);
#endif
	if (by < orig->begy || bx < orig->begx
	    || by + nl > orig->maxy + orig->begy
	    || bx + nc > orig->maxx + orig->begx)
		return (NULL);
	if (nl == 0)
		nl = orig->maxy + orig->begy - by;
	if (nc == 0)
		nc = orig->maxx + orig->begx - bx;
	if ((win = __makenew(nl, nc, by, bx, 1)) == NULL)
		return (NULL);
	win->nextp = orig->nextp;
	orig->nextp = win;
	win->orig = orig;

	/* Initialize flags here so that refresh can also use __set_subwin. */
	for (lp = win->lspace, i = 0; i < win->maxy; i++, lp++)
		lp->flags = 0;
	__set_subwin(orig, win);
	return (win);
}
/*
 * This code is shared with mvwin().
 */
void
__set_subwin(WINDOW *orig, WINDOW *win)
{
	int     i;
	__LINE *lp, *olp;

	win->ch_off = win->begx - orig->begx;
	/* Point line pointers to line space. */
	for (lp = win->lspace, i = 0; i < win->maxy; i++, lp++) {
		win->lines[i] = lp;
		olp = orig->lines[i + win->begy - orig->begy];
		lp->line = &olp->line[win->begx - orig->begx];
		lp->firstchp = &olp->firstch;
		lp->lastchp = &olp->lastch;
		lp->hash = __hash((char *)(void *)lp->line,
		    (int) (win->maxx * __LDATASIZE));
	}

#ifdef DEBUG
	__CTRACE("__set_subwin: win->ch_off = %d\n", win->ch_off);
#endif
}
/*
 * __makenew --
 *	Set up a window buffer and returns a pointer to it.
 */
static WINDOW *
__makenew(nl, nc, by, bx, sub)
	int     by, bx, nl, nc;
	int     sub;
{
	WINDOW			*win;
	__LINE			*lp;
	struct __winlist	*wlp, *wlp2;
	int			 i;


#ifdef	DEBUG
	__CTRACE("makenew: (%d, %d, %d, %d)\n", nl, nc, by, bx);
#endif
	if ((win = malloc(sizeof(*win))) == NULL)
		return (NULL);
#ifdef DEBUG
	__CTRACE("makenew: nl = %d\n", nl);
#endif

	/* Set up line pointer array and line space. */
	if ((win->lines = malloc(nl * sizeof(__LINE *))) == NULL) {
		free(win);
		return NULL;
	}
	if ((win->lspace = malloc(nl * sizeof(__LINE))) == NULL) {
		free(win);
		free(win->lines);
		return NULL;
	}
	/* Don't allocate window and line space if it's a subwindow */
	if (!sub) {
		/*
		 * Allocate window space in one chunk.
		 */
		if ((win->wspace =
			malloc(nc * nl * sizeof(__LDATA))) == NULL) {
			free(win->lines);
			free(win->lspace);
			free(win);
			return NULL;
		}
		/*
		 * Append window to window list.
		 */
		if ((wlp = malloc(sizeof(struct __winlist))) == NULL) {
			free(win->wspace);
			free(win->lines);
			free(win->lspace);
			free(win);
			return NULL;
		}
		wlp->winp = win;
		wlp->nextp = NULL;
		if (__winlistp == NULL)
			__winlistp = wlp;
		else {
			wlp2 = __winlistp;
			while (wlp2->nextp != NULL)
				wlp2 = wlp2->nextp;
			wlp2->nextp = wlp;
		}
		/*
		 * Point line pointers to line space, and lines themselves into
		 * window space.
		 */
		for (lp = win->lspace, i = 0; i < nl; i++, lp++) {
			win->lines[i] = lp;
			lp->line = &win->wspace[i * nc];
			lp->firstchp = &lp->firstch;
			lp->lastchp = &lp->lastch;
			lp->firstch = 0;
			lp->lastch = 0;
		}
	}
#ifdef DEBUG
	__CTRACE("makenew: nc = %d\n", nc);
#endif
	win->cury = win->curx = 0;
	win->maxy = nl;
	win->maxx = nc;

	win->begy = by;
	win->begx = bx;
	win->flags = 0;
	win->wattr = 0;
	win->bchar = ' ';
	win->battr = 0;
	__swflags(win);
#ifdef DEBUG
	__CTRACE("makenew: win->wattr = %0.2o\n", win->wattr);
	__CTRACE("makenew: win->flags = %0.2o\n", win->flags);
	__CTRACE("makenew: win->maxy = %d\n", win->maxy);
	__CTRACE("makenew: win->maxx = %d\n", win->maxx);
	__CTRACE("makenew: win->begy = %d\n", win->begy);
	__CTRACE("makenew: win->begx = %d\n", win->begx);
#endif
	return (win);
}

void
__swflags(WINDOW *win)
{
	win->flags &= ~(__ENDLINE | __FULLWIN | __SCROLLWIN | __LEAVEOK);
	if (win->begx + win->maxx == COLS) {
		win->flags |= __ENDLINE;
		if (win->begx == 0 && win->maxy == LINES && win->begy == 0)
			win->flags |= __FULLWIN;
		if (win->begy + win->maxy == LINES)
			win->flags |= __SCROLLWIN;
	}
}
