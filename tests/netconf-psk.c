/*
 * Copyright (C) 2008 Free Software Foundation
 *
 * Author: Simon Josefsson
 *
 * This file is part of GNUTLS.
 *
 * GNUTLS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * GNUTLS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNUTLS; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <gnutls/gnutls.h>

#include "utils.h"

void
doit (void)
{
  const char *known =
    "\xd6\xa9\x41\x0e\x55\x17\x42\x69\x16\x09"
    "\x1f\xe0\xdc\xab\x7d\x3e\x60\x1b\xf5\x0a";
  gnutls_datum_t key = { NULL, 0 };

  gnutls_global_init ();

  if (gnutls_psk_netconf_derive_key ("password", "psk_identity",
				     "psk_identity_hint", &key) == 0)
    success ("success: gnutls_psk_netconf_derive_key\n");
  else
    fail ("gnutls_psk_netconf_derive_key failure\n");

  if (debug)
    hexprint (key.data, key.size);

  if (key.size == 20 && memcmp (key.data, known, 20) == 0)
    success ("success: match.\n");
  else
    fail ("FAIL: key differ.\n");

  gnutls_free (key.data);

  gnutls_global_deinit ();
}