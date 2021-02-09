/*	$NetBSD: commands.c,v 1.8 2021/02/09 20:24:02 rillig Exp $	*/

/*-
 * Copyright 2009 Brett Lymn <blymn@NetBSD.org>
 *
 * All rights reserved.
 *
 * This code has been donated to The NetBSD Foundation by the Author.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 */

#include <curses.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <err.h>
#include <sys/types.h>
#include "returns.h"
#include "slave.h"
#include "command_table.h"

extern int cmdpipe[2];
extern int slvpipe[2];
extern int initdone;

static void report_type(data_enum_t);
static void report_message(int, const char *);

/*
 * Match the passed command string and execute the associated test
 * function.
 */
void
command_execute(char *func, int nargs, char **args)
{
	size_t i, j;

	i = 0;
	while (i < ncmds) {
		if (strcmp(func, commands[i].name) == 0) {
			/* Check only restricted set of functions is called before
			 * initscr/newterm */
			if(!initdone){
				j = 0;
				while(j < nrcmds) {
					if(strcasecmp(func, restricted_commands[j]) == 0){
						if(strcasecmp(func, "initscr") == 0  ||
							strcasecmp(func, "newterm") == 0)
							initdone = 1;
						/* matched function */
						commands[i].func(nargs, args);
						return;
					}
					j++;
				}
				report_status("YOU NEED TO CALL INITSCR/NEWTERM FIRST");
				return;
			}
			/* matched function */
			commands[i].func(nargs, args);
			return;
		}
		i++;
	}

	report_status("UNKNOWN_FUNCTION");
}

/*
 * Report an pointer value back to the director
 */
void
report_ptr(void *ptr)
{
	char *string;

	if (ptr == NULL)
		asprintf(&string, "NULL");
	else
		asprintf(&string, "%p", ptr);
	report_status(string);
	free(string);
}

/*
 * Report an integer value back to the director
 */
void
report_int(int value)
{
	char *string;

	asprintf(&string, "%d", value);
	report_status(string);
	free(string);
}

/*
 * Report either an ERR or OK back to the director
 */
void
report_return(int status)
{
	if (status == ERR)
		report_type(data_err);
	else if (status == OK)
		report_type(data_ok);
	else if (status == KEY_CODE_YES)
		report_int(status);
	else
		report_status("INVALID_RETURN");
}

/*
 * Report the type back to the director via the command pipe
 */
static void
report_type(data_enum_t return_type)
{
	int type;

	type = return_type;
	if (write(slvpipe[WRITE_PIPE], &type, sizeof(int)) < 0)
		err(1, "command pipe write for status type failed");

}

/*
 * Report the number of returns back to the director via the command pipe
 */
void
report_count(int count)
{
	int type;

	type = data_count;
	if (write(slvpipe[WRITE_PIPE], &type, sizeof(int)) < 0)
		err(1, "command pipe write for count type failed");

	if (write(slvpipe[WRITE_PIPE], &count, sizeof(int)) < 0)
		err(1, "command pipe write for count");
}

/*
 * Report the status back to the director via the command pipe
 */
void
report_status(const char *status)
{
	report_message(data_string, status);
}

/*
 * Report an error message back to the director via the command pipe.
 */
void
report_error(const char *status)
{
	report_message(data_slave_error, status);
}

/*
 * Report the message with the given type back to the director via the
 * command pipe.
 */
static void
report_message(int type, const char *status)
{
	int len;

	len = strlen(status);

	if (write(slvpipe[WRITE_PIPE], &type, sizeof(int)) < 0)
		err(1, "command pipe write for message type failed");

	if (write(slvpipe[WRITE_PIPE], &len, sizeof(int)) < 0)
		err(1, "command pipe write for message length failed");

	if (write(slvpipe[WRITE_PIPE], status, len) < 0)
		err(1, "command pipe write of message data failed");
}

/*
 * Report a string of chtype back to the director via the command pipe.
 */
void
report_byte(chtype c)
{
	chtype string[2];

	string[0] = c;
	string[1] = A_NORMAL | '\0';
	report_nstr(string);
}

/*
 * Report a string of chtype back to the director via the command pipe.
 */
void
report_nstr(chtype *string)
{
	int len, type;
	chtype *p;

	len = 0;
	p = string;

	while ((*p++ & __CHARTEXT) != 0) {
		len++;
	}

	len++; /* add in the termination chtype */
	len *= sizeof(chtype);

	type = data_byte;
	if (write(slvpipe[WRITE_PIPE], &type, sizeof(int)) < 0)
		err(1, "%s: command pipe write for status type failed",
		    __func__);

	if (write(slvpipe[WRITE_PIPE], &len, sizeof(int)) < 0)
		err(1, "%s: command pipe write for status length failed",
		    __func__);

	if (write(slvpipe[WRITE_PIPE], string, len) < 0)
		err(1, "%s: command pipe write of status data failed",
		    __func__);
}

/*
 * Report a cchar_t back to the director via the command pipe.
 */
void
report_cchar(cchar_t c)
{
	int len, type;
	len = sizeof(cchar_t);
	type = data_cchar;

	if (write(slvpipe[WRITE_PIPE], &type, sizeof(int)) < 0)
		err(1, "%s: command pipe write for status type failed",
		    __func__);

	if (write(slvpipe[WRITE_PIPE], &len, sizeof(int)) < 0)
		err(1, "%s: command pipe write for status length failed",
		    __func__);

	if (write(slvpipe[WRITE_PIPE], &c, len) < 0)
		err(1, "%s: command pipe write of status data failed",
		    __func__);
}

/*
 * Report a wchar_t back to the director via the command pipe.
 */
void
report_wchar(wchar_t ch)
{
	wchar_t wstr[2];

	wstr[0] = ch;
	wstr[1] = L'\0';
	report_wstr(wstr);
}


/*
 * Report a string of wchar_t back to the director via the command pipe.
 */
void
report_wstr(wchar_t *wstr)
{
	int len, type;
	wchar_t *p;

	len = 0;
	p = wstr;

	while (*p++ != L'\0')
		len++;

	len++; /* add in the termination chtype */
	len *= sizeof(wchar_t);

	type = data_wchar;
	if (write(slvpipe[WRITE_PIPE], &type, sizeof(int)) < 0)
		err(1, "%s: command pipe write for status type failed",
		    __func__);

	if (write(slvpipe[WRITE_PIPE], &len, sizeof(int)) < 0)
		err(1, "%s: command pipe write for status length failed",
		    __func__);

	if (write(slvpipe[WRITE_PIPE], wstr, len) < 0)
		err(1, "%s: command pipe write of status data failed",
		    __func__);
}

/*
 * Check the number of args we received are what we expect.  Return an
 * error if they do not match.
 */
int
check_arg_count(int nargs, int expected)
{
	if (nargs != expected) {
		report_count(1);
		report_error("INCORRECT_ARGUMENT_NUMBER");
		return(1);
	}

	return(0);
}
