/*********************************************************
 * Copyright (C) 2007 VMware, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation version 2 and no later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 *********************************************************/

/*
 * vsockSocketWrapper.h --
 *
 *    Socket wrapper constants, types and functions.
 */


#ifndef _VSOCK_SOCKET_WRAPPER_H_
#define _VSOCK_SOCKET_WRAPPER_H_


/*
 * Socket states and flags.  Note that MSG_WAITALL is only supported on 2K3,
 * XP-SP2 and above.  Since we currently build for 2K to maintain backwards
 * compatibility, it will always be 0.
 */
#if defined(_WIN32)
#  define MSG_DONTWAIT        0
#  define MSG_NOSIGNAL        0
#  if (_WIN32_WINNT < 0x0502)
#     define MSG_WAITALL      0
#  endif
#endif

#if defined(_WIN32) || defined(VMKERNEL)
#  define SS_FREE             0
#  define SS_UNCONNECTED      1
#  define SS_CONNECTING       2
#  define SS_CONNECTED        3
#  define SS_DISCONNECTING    4
#  define RCV_SHUTDOWN        1
#  define SEND_SHUTDOWN       2
#  define SHUTDOWN_MASK       3
#endif // _WIN32 || VMKERNEL


/*
 * Error codes.
 */
#if defined(_WIN32)
# if !defined(EINTR)
#  define EINTR               WSAEINTR
# endif
# if !defined(EACCES)
#  define EACCES              WSAEACCES
# endif
# if !defined(EFAULT)
#  define EFAULT              WSAEFAULT
# endif
# if !defined(EINVAL)
#  define EINVAL              WSAEINVAL
# endif
#  define EWOULDBLOCK         WSAEWOULDBLOCK
#  define EINPROGRESS         WSAEINPROGRESS
#  define EALREADY            WSAEALREADY
#  define ENOTSOCK            WSAENOTSOCK
#  define EDESTADDRREQ        WSAEDESTADDRREQ
#  define EMSGSIZE            WSAEMSGSIZE
#  define EPROTOTYPE          WSAEPROTOTYPE
#  define ENOPROTOOPT         WSAENOPROTOOPT
#  define EPROTONOSUPPORT     WSAEPROTONOSUPPORT
#  define ESOCKTNOSUPPORT     WSAESOCKTNOSUPPORT
#  define EOPNOTSUPP          WSAEOPNOTSUPP
#  define EPFNOSUPPORT        WSAEPFNOSUPPORT
#  define EAFNOSUPPORT        WSAEAFNOSUPPORT
#  define EADDRINUSE          WSAEADDRINUSE
#  define EADDRNOTAVAIL       WSAEADDRNOTAVAIL
#  define ENETDOWN            WSAENETDOWN
#  define ENETUNREACH         WSAENETUNREACH
#  define ENETRESET           WSAENETRESET
#  define ECONNABORTED        WSAECONNABORTED
#  define ECONNRESET          WSAECONNRESET
#  define ENOBUFS             WSAENOBUFS
#  define EISCONN             WSAEISCONN
#  define ENOTCONN            WSAENOTCONN
#  define ESHUTDOWN           WSAESHUTDOWN
#  define ETIMEDOUT           WSAETIMEDOUT
#  define ECONNREFUSED        WSAECONNREFUSED
#  define EHOSTDOWN           WSAEHOSTDOWN
#  define EHOSTUNREACH        WSAEHOSTUNREACH
#else
#if defined(VMKERNEL)
#  define EINTR               VMK_WAIT_INTERRUPTED
#  define EACCES              VMK_NOACCESS
#  define EFAULT              VMK_INVALID_ADDRESS
#  define EINVAL              VMK_FAILURE
#  define EWOULDBLOCK         VMK_WOULD_BLOCK
#  define EINPROGRESS         VMK_EINPROGRESS
#  define EALREADY            VMK_EALREADY
#  define ENOTSOCK            VMK_NOT_A_SOCKET
#  define EDESTADDRREQ        VMK_EDESTADDRREQ
   /*
    * Do not change EMSGSIZE definition without changing uses of
    * VMK_LIMIT_EXCEEDED in userSocketVmci.c's implementation of recvmsg().
    */
#  define EMSGSIZE            VMK_LIMIT_EXCEEDED
#  define EPROTOTYPE          VMK_NOT_SUPPORTED
#  define ENOPROTOOPT         VMK_NOT_SUPPORTED
#  define EPROTONOSUPPORT     VMK_EPROTONOSUPPORT
#  define ESOCKTNOSUPPORT     VMK_NOT_SUPPORTED
#  define EOPNOTSUPP          VMK_EOPNOTSUPP
#  define EPFNOSUPPORT        VMK_ADDRFAM_UNSUPP
#  define EAFNOSUPPORT        VMK_ADDRFAM_UNSUPP
#  define EADDRINUSE          VMK_EADDRINUSE
#  define EADDRNOTAVAIL       VMK_EADDRNOTAVAIL
#  define ENETDOWN            VMK_ENETDOWN
#  define ENETUNREACH         VMK_ENETUNREACH
#  define ENETRESET           VMK_ENETRESET
#  define ECONNABORTED        VMK_ECONNABORTED
#  define ECONNRESET          VMK_ECONNRESET
#  define ENOBUFS             VMK_NO_MEMORY
#  define ENOMEM              VMK_NO_MEMORY
#  define EISCONN             VMK_ALREADY_CONNECTED
#  define ENOTCONN            VMK_ENOTCONN
#  define ESHUTDOWN           VMK_ESHUTDOWN
#  define ETIMEDOUT           VMK_TIMEOUT
#  define ECONNREFUSED        VMK_ECONNREFUSED
#  define EHOSTDOWN           VMK_EHOSTDOWN
#  define EHOSTUNREACH        VMK_EHOSTUNREACH
#endif // VMKERNEL
#endif // _WIN32


#if defined(_WIN32)
#  define sockerr()           WSAGetLastError()
#  define sockerr2err(_e)     (((_e) < 0) ? -(_e) : (_e))
#  define sockcleanup()       WSACleanup()
   typedef uint32             socklen_t;
   typedef uint32             in_addr_t;
#else // _WIN32
#if defined(VMKERNEL)
#  define SOCKET_ERROR        (-1)
#  define INVALID_SOCKET      ((SOCKET) -1)
#  define sockerr()           errno
#  define sockerr2err(_e)     (_e)
#  define sockcleanup()       do {} while (0)
#  define closesocket(_s)     close((_s))
   typedef int32              SOCKET;
#else
#if defined(linux)
#  define SOCKET_ERROR        (-1)
#  define INVALID_SOCKET      ((SOCKET) -1)
#  define sockerr()           errno
#  define sockerr2err(_e)     (((_e) > 0) ? -(_e) : (_e))
#  define sockcleanup()       do {} while (0)
#  define closesocket(_s)     close((_s))
   typedef int32              SOCKET;
#endif // linux
#endif // VMKERNEL
#endif // _WIN32


/*
 * There is no SS_XXX state equivalent to TCP_LISTEN.  Linux does have a flag
 * __SO_ACCEPTCON which some of the socket implementations use, but it does
 * not fit in the state field (although it is sometimes incorrectly used that
 * way).  So we define our own listen state here for all platforms.
 */
#define SS_LISTEN 255


/*
 * Initialize sockets.  This is really for platforms that do not have
 * on-by-default socket implementations like Windows.
 */
int sockinit(void);


#endif // _VSOCK_SOCKET_WRAPPER_H_

