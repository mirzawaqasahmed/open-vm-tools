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
 * vsockCommon.h --
 *
 *    VSockets common constants, types and functions.
 */


#ifndef _VSOCK_COMMON_H_
#define _VSOCK_COMMON_H_

/*
 * VMCISockGetAFValueInt is defined separately from VMCISock_GetAFValue because
 * it is used in several different contexts. In particular it is called from
 * vsockAddr.c which gets compiled into both our kernel modules as well as
 * the user level vsock library. In the linux kernel we need different behavior
 * than external kernel modules using VMCI Sockets api inside the kernel.
 */

#if defined(_WIN32)
#  define VMCI_SOCKETS_AF_VALUE 28
#  if defined(WINNT_DDK)
#     define _WIN2K_COMPAT_SLIST_USAGE
#     include <ntddk.h>
#     include <windef.h>
#     define _INC_WINDOWS
      /* In the kernel we can't call into the provider. */
#     define VMCISockGetAFValueInt() VMCI_SOCKETS_AF_VALUE
#  else // WINNT_DDK
      /* In userland, just use the normal exported userlevel api. */
#     define VMCISockGetAFValueInt() VMCISock_GetAFValue()
#     include <windows.h>
#  endif // WINNT_DDK
#  define Uint64ToPtr(_ui) ((void *)(uint64)(_ui))
#  define PtrToUint64(_p)  ((uint64)(_p))
#else
#if defined(VMKERNEL)
#  include "uwvmkAPI.h"
#  define VMCI_SOCKETS_AF_VALUE AF_VMCI /* Defined in uwvmkAPI.h. */
   /* The address family is fixed in the vmkernel. */
#  define VMCISockGetAFValueInt() VMCI_SOCKETS_AF_VALUE
#  include "vmciHostKernelAPI.h"
#  define Uint64ToPtr(_ui) ((void *)(uint64)(_ui))
#  define PtrToUint64(_p)  ((uint64)(_p))
#else
#if defined(linux)
#  if defined(__KERNEL__)
   /* Include compat_page.h now so PAGE_SIZE and friends don't get redefined. */
#     include "driver-config.h"
#     include "compat_page.h"
#     if defined(VMX86_TOOLS)
#        include "vmci_queue_pair.h"
#     endif
    /*
     * In the kernel we call back into af_vsock.c to get the address family
     * being used.  Otherwise an ioctl(2) is performed (see vmci_sockets.h).
     */
      extern int VSockVmci_GetAFValue(void);
#     define VMCISockGetAFValueInt() VSockVmci_GetAFValue()
#  else // __KERNEL__
      /* In userland, just use the normal exported userlevel api. */
#     define VMCISockGetAFValueInt() VMCISock_GetAFValue()
#  endif
#else
#if defined(__APPLE__)
#  if defined(KERNEL)
#     include "vmci_queue_pair.h"

/*
 * XXX: These defines are NOT 64 bit safe - they need to be revisited as part
 * of any work to support 64 bit kernels.
 */
#     define Uint64ToPtr(_ui) ((void *)(uint32)(_ui))
#     define PtrToUint64(_p)  ((uint64)(_p))

#     define VMCI_SOCKETS_AF_VALUE PF_SYSTEM
#     define VMCISockGetAFValueInt() VMCI_SOCKETS_AF_VALUE
#  else // KERNEL
#     define VMCISockGetAFValueInt() VMCISock_GetAFValue()
#  endif // KERNEL
#endif // __APPLE__
#endif // linux
#endif // VMKERNEL
#endif // _WIN32

#include "vmware.h"
#include "vmware_pack_init.h"
#include "vmci_defs.h"
#include "vmci_call_defs.h"
#include "vmci_sockets_int.h"
#include "vmci_sockets.h"

#if defined(WINNT_DDK)
#  include <winsock2.h>
#endif // WINNT_DDK

#include "vsockAddr.h"
#include "vsockSocketWrapper.h"


/* Memory allocation flags. */
#define VSOCK_MEMORY_NORMAL   0
#define VSOCK_MEMORY_ATOMIC   (1 << 0)
#define VSOCK_MEMORY_NONPAGED (1 << 1)


#endif // _VSOCK_COMMON_H_

