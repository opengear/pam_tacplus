/* messages.c - Various messages returned to user.
 * 
 * Copyright (C) 2010, Pawel Krawczyk <pawel.krawczyk@hush.com> and
 * Jeroen Nijhof <jeroen@jeroennijhof.nl>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program - see the file COPYING.
 *
 * See `CHANGES' file for revision history.
 */

__attribute__((__visibility__("hidden"))) const
char *protocol_err_msg = "(Protocol error)";
__attribute__((__visibility__("hidden"))) const
char *authen_syserr_msg = "(Authentication system error)";
__attribute__((__visibility__("hidden"))) const
char *author_ok_msg = "(Service granted)";
__attribute__((__visibility__("hidden"))) const
char *author_fail_msg = "(Service not allowed)";
__attribute__((__visibility__("hidden"))) const
char *author_err_msg = "(Service not allowed. Server error)";
__attribute__((__visibility__("hidden"))) const
char *author_syserr_msg = "(Authorization system error)";
__attribute__((__visibility__("hidden"))) const
char *acct_ok_msg = "(Accounted ok)";
__attribute__((__visibility__("hidden"))) const
char *acct_fail_msg = "(Accounting failed)";
__attribute__((__visibility__("hidden"))) const
char *acct_err_msg = "(Accounting failed. Server error)";
__attribute__((__visibility__("hidden"))) const
char *acct_syserr_msg = "(Accounting system error)";
