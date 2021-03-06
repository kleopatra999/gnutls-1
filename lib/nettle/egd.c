/* rndegd.c  -	interface to the EGD
 *	Copyright (C) 1999, 2000, 2002, 2003, 2010 Free Software Foundation, Inc.
 *
 * This file is part of Libgcrypt.
 *
 * Libgcrypt is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Libgcrypt is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "egd.h"

#include <gnutls_errors.h>

#ifndef offsetof
#define offsetof(type, member) ((size_t) &((type *)0)->member)
#endif

static int egd_socket = -1;

static int
do_write( int fd, void *buf, size_t nbytes )
{
  size_t nleft = nbytes;
  int nwritten;
  
  while( nleft > 0 ) 
    {
      nwritten = write( fd, buf, nleft);
      if( nwritten < 0 )
        {
          if( errno == EINTR )
            continue;
          return -1;
	}
      nleft -= nwritten;
      buf = (char*)buf + nwritten;
    }
  return 0;
}

static int
do_read( int fd, void *buf, size_t nbytes )
{
  int n, nread = 0;

  do
    {
      do
        {
          n = read(fd, (char*)buf + nread, nbytes );
        } 
      while( n == -1 && errno == EINTR );
      if( n == -1)
        return nread? nread:-1;
      if( n == 0)
        return -1;
      nread += n;
      nbytes -= n;
    } 
  while( nread < nbytes );
  return nread;
}

static const char* egd_names[] = { 
  "/var/run/egd-pool", 
  "/dev/egd-pool", 
  "/etc/egd-pool", 
  "/etc/entropy", 
  "/var/run/entropy", 
  "/dev/entropy",
  NULL 
};

static const char* find_egd_name(void)
{
int i = 0;
struct stat st;

  do {
    if (stat(egd_names[i], &st) != 0) continue;
    
    if (st.st_mode & S_IFSOCK) { /* found */
      return egd_names[i];
    }
  
  } while(egd_names[++i] != NULL);
  
  return NULL;
}

/* Connect to the EGD and return the file descriptor.  Return -1 on
   error.  With NOFAIL set to true, silently fail and return the
   error, otherwise print an error message and die. */
int _rndegd_connect_socket (void)
{
  int fd;
  const char *name;
  struct sockaddr_un addr;
  int addr_len;

  if (egd_socket != -1)
    {
      close (egd_socket);
      egd_socket = -1;
    }

  name = find_egd_name();

  if (strlen(name)+1 >= sizeof addr.sun_path) 
    {
      _gnutls_debug_log ("EGD socketname is too long\n");
      return -1;
    }
  
  memset( &addr, 0, sizeof addr );
  addr.sun_family = AF_LOCAL;
  strcpy( addr.sun_path, name );	  
  addr_len = (offsetof( struct sockaddr_un, sun_path )
              + strlen( addr.sun_path ));
  
  fd = socket(AF_LOCAL, SOCK_STREAM, 0);
  if (fd == -1)
    {
      _gnutls_debug_log("can't create unix domain socket: %s\n", strerror(errno) );
      return -1;
    }
  else if (connect (fd, (struct sockaddr*)&addr, addr_len) == -1)
    {
      _gnutls_debug_log("can't connect to EGD socket `%s': %s\n",
		  name, strerror(errno) );
      close (fd);
      fd = -1;
    }

  if (fd != -1)
    egd_socket = fd;
  return fd;
}

/****************
 * Note: We always use the highest level.
 * To boost the performance we may want to add some
 * additional code for level 1
 *
 * Using a level of 0 should never block and better add nothing
 * to the pool.  So this is just a dummy for EGD.
 */
int _rndegd_read (int * fd, void* _output, size_t _length)
{
  int n;
  uint8_t buffer[256+2];
  int nbytes;
  int do_restart = 0;
  unsigned char* output = _output;
  size_t length = _length;

  if( !length )
    return 0;


 restart:
  if (*fd == -1 || do_restart)
    *fd = _rndegd_connect_socket ();

  do_restart = 0;

  nbytes = length < 255 ? length : 255;
  /* First time we do it with a non blocking request */
  buffer[0] = 1; /* non blocking */
  buffer[1] = nbytes;

  if( do_write( *fd, buffer, 2 ) == -1 )
    _gnutls_debug_log("can't write to the EGD: %s\n", strerror(errno) );

  n = do_read( *fd, buffer, 1 );
  if( n == -1 )
    {
      _gnutls_debug_log("read error on EGD: %s\n", strerror(errno));
      do_restart = 1;
      goto restart;
    }

  n = buffer[0];
  if( n )
    {
      n = do_read( *fd, buffer, n );
      if( n == -1 )
        {
          _gnutls_debug_log("read error on EGD: %s\n", strerror(errno));
          do_restart = 1;
          goto restart;
	}

      if (n > length)
        {
          _gnutls_debug_log("read error on EGD: returned more bytes!\n");
          n = length;
        }

      memcpy(output, buffer, n);
      output += n;
      length -= n;
    }

  while( length )
    {
      nbytes = length < 255? length : 255;

      buffer[0] = 2; /* blocking */
      buffer[1] = nbytes;
      if( do_write( *fd, buffer, 2 ) == -1 )
        _gnutls_debug_log("can't write to the EGD: %s\n", strerror(errno) );
      n = do_read( *fd, buffer, nbytes );
      if( n == -1 )
        {
          _gnutls_debug_log("read error on EGD: %s\n", strerror(errno));
          do_restart = 1;
          goto restart;
	}

      if (n > length)
        {
          _gnutls_debug_log("read error on EGD: returned more bytes!\n");
          n = length;
        }
      
      memcpy(output, buffer, n);
      output += n;
      length -= n;
    }

  return _length; /* success */
}
