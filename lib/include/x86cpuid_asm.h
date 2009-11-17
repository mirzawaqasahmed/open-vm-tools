/*********************************************************
 * Copyright (C) 2003-2009 VMware, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation version 2.1 and no later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the Lesser GNU General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA.
 *
 *********************************************************/

/*********************************************************
 * The contents of this file are subject to the terms of the Common
 * Development and Distribution License (the "License") version 1.0
 * and no later version.  You may not use this file except in
 * compliance with the License.
 *
 * You can obtain a copy of the License at
 *         http://www.opensource.org/licenses/cddl1.php
 *
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 *********************************************************/

/*
 * x86cpuid_asm.h
 *
 *	CPUID-related assembly functions.
 */

#ifndef _X86CPUID_ASM_H_
#define _X86CPUID_ASM_H_

#define INCLUDE_ALLOW_USERLEVEL
#define INCLUDE_ALLOW_VMMEXT
#define INCLUDE_ALLOW_MODULE
#define INCLUDE_ALLOW_VMMON
#define INCLUDE_ALLOW_VMNIXMOD
#define INCLUDE_ALLOW_VMK_MODULE
#define INCLUDE_ALLOW_VMKERNEL
#define INCLUDE_ALLOW_DISTRIBUTE
#define INCLUDE_ALLOW_VMCORE
#include "includeCheck.h"

#include "vm_basic_asm.h"
#include "x86cpuid.h"


/*
 * x86-64 windows doesn't support inline asm so we have to use these
 * intrinsic functions defined in the compiler.  Not all of these are well
 * documented.  There is an array in the compiler dll (c1.dll) which has
 * an array of the names of all the intrinsics minus the leading
 * underscore.  Searching around in the ntddk.h file can also be helpful.
 *
 * The declarations for the intrinsic functions were taken from the DDK. 
 * Our declarations must match the ddk's otherwise the 64-bit c++ compiler
 * will complain about second linkage of the intrinsic functions.
 * We define the intrinsic using the basic types corresponding to the 
 * Windows typedefs. This avoids having to include windows header files
 * to get to the windows types.
 */
#ifdef _MSC_VER
#ifdef __cplusplus
extern "C" {
#endif
#ifdef VM_X86_64
/*
 * intrinsic functions only supported by x86-64 windows as of 2k3sp1
 */
void             __cpuid(unsigned int*, unsigned int);
#pragma intrinsic(__cpuid)
#endif /* VM_X86_64 */

#ifdef __cplusplus
}
#endif
#endif /* _MSC_VER */


#ifdef __GNUC__ // {

/*
 * Checked against the Intel manual and GCC --hpreg
 * 
 * Need __volatile__ and "memory" since CPUID has a synchronizing effect.
 * The CPUID may also change at runtime (APIC flag, etc).
 *
 */

static INLINE void
__GET_CPUID(int eax,         // IN
            CPUIDRegs *regs) // OUT
{
   __asm__ __volatile__(
#if defined __PIC__ && !vm_x86_64 // %ebx is reserved by the compiler.
      "movl %%ebx, %1"  "\n\t"
      "cpuid"           "\n\t"
      "xchgl %%ebx, %1"
      : "=a" (regs->eax), "=&rm" (regs->ebx), "=c" (regs->ecx), "=d" (regs->edx)
#else
      "cpuid"
      : "=a" (regs->eax), "=b" (regs->ebx), "=c" (regs->ecx), "=d" (regs->edx)
#endif
      : "a" (eax)
      : "memory"
   );
}

static INLINE void
__GET_CPUID2(int eax,         // IN
             int ecx,         // IN
             CPUIDRegs *regs) // OUT
{
   __asm__ __volatile__(
#if defined __PIC__ && !vm_x86_64 // %ebx is reserved by the compiler.
      "movl %%ebx, %1"  "\n\t"
      "cpuid"           "\n\t"
      "xchgl %%ebx, %1"
      : "=a" (regs->eax), "=&rm" (regs->ebx), "=c" (regs->ecx), "=d" (regs->edx)
#else
      "cpuid"
      : "=a" (regs->eax), "=b" (regs->ebx), "=c" (regs->ecx), "=d" (regs->edx)
#endif
      : "a" (eax), "c" (ecx)
      : "memory"
   );
}

static INLINE uint32
__GET_EAX_FROM_CPUID(int eax) // IN
{
#if defined __PIC__ && !vm_x86_64 // %ebx is reserved by the compiler.
   uint32 ebx;

   __asm__ __volatile__(
      "movl %%ebx, %1"  "\n\t"
      "cpuid"           "\n\t"
      "xchgl %%ebx, %1"
      : "=a" (eax), "=&rm" (ebx)
      : "a" (eax)
      : "memory", "%ecx", "%edx"
   );
#else
   __asm__ __volatile__(
      "cpuid"
      : "=a" (eax)
      : "a" (eax)
      : "memory", "%ebx", "%ecx", "%edx"
   );
#endif

   return eax;
}

static INLINE uint32
__GET_EBX_FROM_CPUID(int eax) // IN
{
   uint32 ebx;

   __asm__ __volatile__(
#if defined __PIC__ && !vm_x86_64 // %ebx is reserved by the compiler.
      "movl %%ebx, %1"  "\n\t"
      "cpuid"           "\n\t"
      "xchgl %%ebx, %1"
      : "=a" (eax), "=&rm" (ebx)
#else
      "cpuid"
      : "=a" (eax), "=b" (ebx)
#endif
      : "a" (eax)
      : "memory", "%ecx", "%edx"
   );

   return ebx;
}

static INLINE uint32
__GET_ECX_FROM_CPUID(int eax) // IN
{
   uint32 ecx;
#if defined __PIC__ && !vm_x86_64 // %ebx is reserved by the compiler.
   uint32 ebx;

   __asm__ __volatile__(
      "movl %%ebx, %1"  "\n\t"
      "cpuid"           "\n\t"
      "xchgl %%ebx, %1"
      : "=a" (eax), "=&rm" (ebx), "=c" (ecx)
      : "a" (eax)
      : "memory", "%edx"
   );
#else

   __asm__ __volatile__(
      "cpuid"
      : "=a" (eax), "=c" (ecx)
      : "a" (eax)
      : "memory", "%ebx", "%edx"
   );
#endif

   return ecx;
}

static INLINE uint32
__GET_EDX_FROM_CPUID(int eax) // IN
{
   uint32 edx;
#if defined __PIC__ && !vm_x86_64 // %ebx is reserved by the compiler.
   uint32 ebx;

   __asm__ __volatile__(
      "movl %%ebx, %1"  "\n\t"
      "cpuid"           "\n\t"
      "xchgl %%ebx, %1"
      : "=a" (eax), "=&rm" (ebx), "=d" (edx)
      : "a" (eax)
      : "memory", "%ecx"
   );
#else

   __asm__ __volatile__(
      "cpuid"
      : "=a" (eax), "=d" (edx)
      : "a" (eax)
      : "memory", "%ebx", "%ecx"
   );
#endif

   return edx;
}


static INLINE uint32
__GET_EAX_FROM_CPUID4(int ecx) // IN
{
   uint32 eax;
#if defined __PIC__ && !vm_x86_64 // %ebx is reserved by the compiler.
   uint32 ebx;

   __asm__ __volatile__(
      "movl %%ebx, %1"  "\n\t"
      "cpuid"           "\n\t"
      "xchgl %%ebx, %1"
      : "=a" (eax), "=&rm" (ebx), "=c" (ecx)
      : "a" (4), "c" (ecx)
      : "memory", "%edx"
   );
#else

   __asm__ __volatile__(
      "cpuid"
      : "=a" (eax), "=c" (ecx)
      : "a" (4), "c" (ecx)
      : "memory", "%ebx", "%edx"
   );
#endif

   return eax;
}

#elif defined(_MSC_VER) // } {

static INLINE void
__GET_CPUID(int input, CPUIDRegs *regs)
{
#ifdef VM_X86_64
   __cpuid((unsigned int *)regs, input);
#else
   __asm push esi
   __asm push ebx
   __asm push ecx
   __asm push edx

   __asm mov  eax, input
   __asm mov  esi, regs
   __asm _emit 0x0f __asm _emit 0xa2
   __asm mov 0x0[esi], eax
   __asm mov 0x4[esi], ebx
   __asm mov 0x8[esi], ecx
   __asm mov 0xC[esi], edx

   __asm pop edx
   __asm pop ecx
   __asm pop ebx
   __asm pop esi
#endif
}

#ifdef VM_X86_64

/*
 * No inline assembly in Win64. Implemented in bora/lib/user in
 * cpuidMasm64.asm.
 */

extern void
__GET_CPUID2(int inputEax, int inputEcx, CPUIDRegs *regs);

#else // VM_X86_64

static INLINE void
__GET_CPUID2(int inputEax, int inputEcx, CPUIDRegs *regs)
{
   __asm push esi
   __asm push ebx
   __asm push ecx
   __asm push edx

   __asm mov  eax, inputEax
   __asm mov  ecx, inputEcx
   __asm mov  esi, regs
   __asm _emit 0x0f __asm _emit 0xa2
   __asm mov 0x0[esi], eax
   __asm mov 0x4[esi], ebx
   __asm mov 0x8[esi], ecx
   __asm mov 0xC[esi], edx

   __asm pop edx
   __asm pop ecx
   __asm pop ebx
   __asm pop esi
}
#endif

static INLINE uint32
__GET_EAX_FROM_CPUID(int input)
{
#ifdef VM_X86_64
   CPUIDRegs regs;
   __cpuid((unsigned int *)&regs, input);
   return regs.eax;
#else
   uint32 output;

   //NOT_TESTED();
   __asm push ebx
   __asm push ecx
   __asm push edx

   __asm mov  eax, input
   __asm _emit 0x0f __asm _emit 0xa2
   __asm mov  output, eax

   __asm pop edx
   __asm pop ecx
   __asm pop ebx

   return output;
#endif
}

static INLINE uint32
__GET_EBX_FROM_CPUID(int input)
{
#ifdef VM_X86_64
   CPUIDRegs regs;
   __cpuid((unsigned int *)&regs, input);
   return regs.ebx;
#else
   uint32 output;

   //NOT_TESTED();
   __asm push ebx
   __asm push ecx
   __asm push edx

   __asm mov  eax, input
   __asm _emit 0x0f __asm _emit 0xa2
   __asm mov  output, ebx

   __asm pop edx
   __asm pop ecx
   __asm pop ebx

   return output;
#endif
}

static INLINE uint32
__GET_ECX_FROM_CPUID(int input)
{
#ifdef VM_X86_64
   CPUIDRegs regs;
   __cpuid((unsigned int *)&regs, input);
   return regs.ecx;
#else
   uint32 output;

   //NOT_TESTED();
   __asm push ebx
   __asm push ecx
   __asm push edx

   __asm mov  eax, input
   __asm _emit 0x0f __asm _emit 0xa2
   __asm mov  output, ecx

   __asm pop edx
   __asm pop ecx
   __asm pop ebx

   return output;
#endif
}

static INLINE uint32
__GET_EDX_FROM_CPUID(int input)
{
#ifdef VM_X86_64
   CPUIDRegs regs;
   __cpuid((unsigned int *)&regs, input);
   return regs.edx;
#else
   uint32 output;

   //NOT_TESTED();
   __asm push ebx
   __asm push ecx
   __asm push edx

   __asm mov  eax, input
   __asm _emit 0x0f __asm _emit 0xa2
   __asm mov  output, edx

   __asm pop edx
   __asm pop ecx
   __asm pop ebx

   return output;
#endif
}

#ifdef VM_X86_64

/*
 * No inline assembly in Win64. Implemented in bora/lib/user in
 * cpuidMasm64.asm.
 */

extern uint32
__GET_EAX_FROM_CPUID4(int inputEcx);

#else // VM_X86_64

static INLINE uint32
__GET_EAX_FROM_CPUID4(int inputEcx)
{
   uint32 output;

   //NOT_TESTED();
   __asm push ebx
   __asm push ecx
   __asm push edx

   __asm mov  eax, 4
   __asm mov  ecx, inputEcx
   __asm _emit 0x0f __asm _emit 0xa2
   __asm mov  output, eax

   __asm pop edx
   __asm pop ecx
   __asm pop ebx

   return output;
}

#endif // VM_X86_64

#else // }
#error 
#endif

#define CPUID_FOR_SIDE_EFFECTS() ((void)__GET_EAX_FROM_CPUID(0))

static INLINE void
__GET_CPUID4(int inputEcx, CPUIDRegs *regs)
{
   __GET_CPUID2(4, inputEcx, regs);
}

/* The first parameter is used as an rvalue and then as an lvalue. */
#define GET_CPUID(_ax, _bx, _cx, _dx) { \
   CPUIDRegs regs;                      \
   __GET_CPUID(_ax, &regs);             \
   _ax = regs.eax;                      \
   _bx = regs.ebx;                      \
   _cx = regs.ecx;                      \
   _dx = regs.edx;                      \
}


#endif
