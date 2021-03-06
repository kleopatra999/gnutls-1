/*
 * Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2010 Free
 * Software Foundation, Inc.
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of GnuTLS.
 *
 * The GnuTLS is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA
 *
 */

/* This file contains the functions needed for 64 bit integer support in
 * TLS, and functions which ease the access to TLS vectors (data of given size).
 */

#include <gnutls_int.h>
#include <gnutls_num.h>
#include <gnutls_errors.h>

#include <byteswap.h>

/* This function will add one to uint64 x.
 * Returns 0 on success, or -1 if the uint64 max limit
 * has been reached.
 */
int
_gnutls_uint64pp (uint64 * x)
{
  register int i, y = 0;

  for (i = 7; i >= 0; i--)
    {
      y = 0;
      if (x->i[i] == 0xff)
	{
	  x->i[i] = 0;
	  y = 1;
	}
      else
	x->i[i]++;

      if (y == 0)
	break;
    }
  if (y != 0)
    return -1;			/* over 64 bits! WOW */

  return 0;
}

/* This function will add one to uint48 x.
 * Returns 0 on success, or -1 if the uint48 max limit
 * has been reached.
 */
int
_gnutls_uint48pp (uint48 * x)
{
  register int i, y = 0;

  for (i = 5; i >= 0; i--)
    {
      y = 0;
      if (x->i[i] == 0xff)
	{
	  x->i[i] = 0;
	  y = 1;
	}
      else
	x->i[i]++;

      if (y == 0)
	break;
    }
  if (y != 0)
    return -1;			/* over 48 bits! meh... */

  return 0;
}

uint32_t
_gnutls_uint24touint32 (uint24 num)
{
  uint32_t ret = 0;

  ((uint8_t *) & ret)[1] = num.pint[0];
  ((uint8_t *) & ret)[2] = num.pint[1];
  ((uint8_t *) & ret)[3] = num.pint[2];
  return ret;
}

uint24
_gnutls_uint32touint24 (uint32_t num)
{
  uint24 ret;

  ret.pint[0] = ((uint8_t *) & num)[1];
  ret.pint[1] = ((uint8_t *) & num)[2];
  ret.pint[2] = ((uint8_t *) & num)[3];
  return ret;

}

uint64_t
_gnutls_uint48touint64 (uint48 num)
{
  uint64_t ret=0;

  ((uint8_t *) & ret)[2] = num.i[0];
  ((uint8_t *) & ret)[3] = num.i[1];
  ((uint8_t *) & ret)[4] = num.i[2];
  ((uint8_t *) & ret)[5] = num.i[3];
  ((uint8_t *) & ret)[6] = num.i[4];
  ((uint8_t *) & ret)[7] = num.i[5];
  return ret;
}

/* data should be at least 3 bytes */
uint32_t
_gnutls_read_uint24 (const opaque * data)
{
  uint32_t res;
  uint24 num;

  num.pint[0] = data[0];
  num.pint[1] = data[1];
  num.pint[2] = data[2];

  res = _gnutls_uint24touint32 (num);
#ifndef WORDS_BIGENDIAN
  res = bswap_32 (res);
#endif
  return res;
}

void
_gnutls_write_uint24 (uint32_t num, opaque * data)
{
  uint24 tmp;

#ifndef WORDS_BIGENDIAN
  num = bswap_32 (num);
#endif
  tmp = _gnutls_uint32touint24 (num);

  data[0] = tmp.pint[0];
  data[1] = tmp.pint[1];
  data[2] = tmp.pint[2];
}

uint32_t
_gnutls_read_uint32 (const opaque * data)
{
  uint32_t res;

  memcpy (&res, data, sizeof (uint32_t));
#ifndef WORDS_BIGENDIAN
  res = bswap_32 (res);
#endif
  return res;
}

void
_gnutls_write_uint32 (uint32_t num, opaque * data)
{

#ifndef WORDS_BIGENDIAN
  num = bswap_32 (num);
#endif
  memcpy (data, &num, sizeof (uint32_t));
}

uint16_t
_gnutls_read_uint16 (const opaque * data)
{
  uint16_t res;
  memcpy (&res, data, sizeof (uint16_t));
#ifndef WORDS_BIGENDIAN
  res = bswap_16 (res);
#endif
  return res;
}

void
_gnutls_write_uint16 (uint16_t num, opaque * data)
{

#ifndef WORDS_BIGENDIAN
  num = bswap_16 (num);
#endif
  memcpy (data, &num, sizeof (uint16_t));
}

uint32_t
_gnutls_conv_uint32 (uint32_t data)
{
#ifndef WORDS_BIGENDIAN
  return bswap_32 (data);
#else
  return data;
#endif
}

uint16_t
_gnutls_conv_uint16 (uint16_t data)
{
#ifndef WORDS_BIGENDIAN
  return bswap_16 (data);
#else
  return data;
#endif
}

uint32_t
_gnutls_uint64touint32 (const uint64 * num)
{
  uint32_t ret;

  memcpy (&ret, &num->i[4], 4);
#ifndef WORDS_BIGENDIAN
  ret = bswap_32 (ret);
#endif

  return ret;
}

uint64_t
_gnutls_read_uint48 (const opaque * data)
{
  uint64_t ret;
  uint48 num;

  memcpy(num.i, data, 6);

  ret = _gnutls_uint48touint64 (num);
#ifndef WORDS_BIGENDIAN
  ret = bswap_64 (ret);
#endif

  return ret;
}
