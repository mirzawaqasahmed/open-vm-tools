/*********************************************************
 * Copyright (C) 1998 VMware, Inc. All rights reserved.
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

#ifdef _WIN32
#error "This file should not be compiled under Win32"
#endif

#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef __APPLE__
#include <sys/utsname.h>
#endif

#include "vm_assert.h"
#include "str.h"
#include "util.h"
#include "syncWaitQ.h"
#include "posix.h"


/*
 * syncWaitQPosix.c --
 *
 *      Kernel (2.4, it changed a bit in 2.5) wait-queue semantics in
 *      userland, yeah baby! -- hpreg
 *
 *      The semantics of this implementation are as follows:
 *
 *      o Client threads can add themselves to a waitqueue object and
 *      receive a pollable handle via a call to SyncWaitQ_Add
 *
 *      o When the waitqueue is woken up, each handle that was
 *      previously obtained via a call to SyncWaitQ_Add becomes
 *      signalled and remains so until it is removed via a call to
 *      SyncWaitQ_Remove. Any calls to SyncWaitQ_Add, after the queue
 *      has been woken up, will return fresh, unsignalled handles.
 * 
 */

/*
 * TODO:
 * Run this test code on 2.0 and 2.2 kernels
 * 
 * kernel                        user
 * ------                        ----
 * wait queue                    FIFO
 * 
 * state=INT, add to wait queue  open FIFO in read mode
 * check sleep condition         check sleep condition
 * schedule                      poll
 * remove from wait queue        close
 * 
 * wake_up wait queue            open FIFO in write mode + close
 * 
 *  I put a lot of thought into this, we may want to patent it just
 *  for fun. I don't know of any prior art --hpreg 
 */

/*
 * How to use this to avoid races:
 * 
 * Some process initializes the wait queue in shared mem with
 * SyncWaitQ_Init(&wq)
 * 
 * P0 does:
 *
 * for (;;) {
 *    fd = SyncWaitQ_Add(&wq);
 *    if (some condition) {
 *       SyncWaitQ_Remove(&wq, fd);
 *       break;
 *    }
 *    poll(fd, ...);
 *    SyncWaitQ_Remove(&wq, fd);
 * }
 * 
 * P1 does:
 *
 * some condition = TRUE;
 * SyncWaitQ_WakeUp(&wq);
 * 
 * Note2: if there is activity on a wait-queue fd, it basically means
 * that the fd becomes unusable after that (it keeps returning
 * POLLHUP), and you need a new one. Actually I take this back, we can
 * reopen and then dup2, so we keep the same fd number and there is no
 * need for the client code to re-register the new fd with its poll
 * loop. I knew this trick would be useful one day :)
 *
 */

typedef union {
   uint32 fd[2];
   uint64 i64;
} HandlesAsI64;


static INLINE Bool SyncWaitQWakeUpNamed(SyncWaitQ *that);
static INLINE Bool SyncWaitQWakeUpAnon(SyncWaitQ *that);
static INLINE char *SyncWaitQMakeName(const char *path, uint64 seq);


#if __APPLE__
/*
 * See VMware bug 116441: we workaround Apple bug 4751096 (calling close and
 * dup simultaneously on the same fd makes the Mac OS kernel panic when the
 * application exits) by serializing these calls.
 *
 * The bug is fixed in Leopard GA (build 9A581).
 */

enum {
   WORKAROUND_UNKNOWN,
   WORKAROUND_NO,
   WORKAROUND_YES,
};
static Atomic_Int workaround = { WORKAROUND_UNKNOWN, };


/*
 *-----------------------------------------------------------------------------
 *
 * SyncWaitQInit --
 *
 *      If the workaround is needed, initialize the wait queue's mutex.
 *
 * Results:
 *      On success: 0.
 *      On failure: errno.
 *
 * Side effects:
 *      None
 *
 *---------------------------------------------------------------------------- 
 */

static int
SyncWaitQInit(SyncWaitQ *that) // IN
{
   if (UNLIKELY(Atomic_ReadInt(&workaround) == WORKAROUND_UNKNOWN)) {
      struct utsname u;
      unsigned int major;

      /*
       * We purposedly do not use Hostinfo_OSVersion() to avoid introducing a
       * library dependency just for a workaround.
       */
      Atomic_ReadIfEqualWriteInt(&workaround, WORKAROUND_UNKNOWN,
         (   uname(&u) == -1
          || sscanf(u.release, "%u.", &major) != 1
          || major < 9) ? WORKAROUND_YES : WORKAROUND_NO);
   }

   ASSERT(Atomic_ReadInt(&workaround) != WORKAROUND_UNKNOWN);
   return Atomic_ReadInt(&workaround) == WORKAROUND_YES
             ? pthread_mutex_init(&that->mutex, NULL) : 0;
}


/*
 *-----------------------------------------------------------------------------
 *
 * SyncWaitQLock --
 *
 *      If the workaround is needed, grab the wait queue's mutex.
 *
 * Results:
 *      None
 *
 * Side effects:
 *      None
 *
 *---------------------------------------------------------------------------- 
 */

static void
SyncWaitQLock(SyncWaitQ *that) // IN
{
   ASSERT(Atomic_ReadInt(&workaround) != WORKAROUND_UNKNOWN);
   if (Atomic_ReadInt(&workaround) == WORKAROUND_YES) {
      int result;

      result = pthread_mutex_lock(&that->mutex);
      ASSERT(!result);
   }
}


/*
 *-----------------------------------------------------------------------------
 *
 * SyncWaitQUnlock --
 *
 *      If the workaround is needed, release the wait queue's mutex.
 *
 * Results:
 *      None
 *
 * Side effects:
 *      None
 *
 *---------------------------------------------------------------------------- 
 */

static void
SyncWaitQUnlock(SyncWaitQ *that) // IN
{
   ASSERT(Atomic_ReadInt(&workaround) != WORKAROUND_UNKNOWN);
   if (Atomic_ReadInt(&workaround) == WORKAROUND_YES) {
      int result;

      result = pthread_mutex_unlock(&that->mutex);
      ASSERT(!result);
   }
}
#endif


/*
 *-----------------------------------------------------------------------------
 *
 * SyncWaitQPanicOnFdLimit --
 *
 *      Panic if 'error' corresponds to an fd limit being reached. See bug
 *      72108. Just like an out-of-memory condition, an out-of-fd condition is
 *      pretty much unrecoverable. The only thing we can do is help our users
 *      diagnose the problem.
 *
 * Results:
 *      None
 *
 * Side effects:
 *      None
 *
 *---------------------------------------------------------------------------- 
 */

static void
SyncWaitQPanicOnFdLimit(int error) // IN
{
   switch (error) {
   case EMFILE:
      Panic("SyncWaitQ: Too many file descriptors are in use by the "
            "process.\n");
      break;

   case ENFILE:
      Panic("SyncWaitQ: The system limit on the total number of open files "
            "has been reached.\n");
      break;

   default:
      break;
   }
}


/*
 *----------------------------------------------------------------------
 *
 * SyncWaitQ_Init --
 *
 *      Initializes a waitqueue structure.
 *
 *      On Win32 the 'path' is the name of the waitqueue in the kernel
 *      namespace (actually a Win32 event).
 *
 *      On Posix the 'path' is the file path to a fifo. The fifo
 *      object itself does not have to exist, but its parent directory
 *      must exist.
 *
 *      If 'path' is NULL, then the waitqueue object is anonymous
 *
 *      It is illegal for the 'path' argument to be empty string.
 *
 * Results:
 *      TRUE on success and FALSE otherwise.
 *
 * Side effects:
 *      On Posix, when named creates a fifo.
 *
 *---------------------------------------------------------------------- 
 */

Bool
SyncWaitQ_Init(SyncWaitQ *that,   // OUT
	       char const *path)  // IN/OPT
{
   ASSERT(that);
   ASSERT(!path || path[0]);

   memset(that, 0, sizeof(SyncWaitQ));

   if (!path) {
      /*
       * Anonymous
       */

      HandlesAsI64 rwHandles;

      if (pipe(rwHandles.fd) < 0) {
         SyncWaitQPanicOnFdLimit(errno);
	 return FALSE;
      }

      if (   fcntl(rwHandles.fd[0], F_SETFL, O_RDONLY | O_NONBLOCK) < 0
          || fcntl(rwHandles.fd[1], F_SETFL, O_WRONLY | O_NONBLOCK) < 0
#if __APPLE__
          || SyncWaitQInit(that)
#endif
                                                                       ) {
	 close(rwHandles.fd[0]);
	 close(rwHandles.fd[1]);
	 return FALSE;
      }
      
      Atomic_Write64(&that->rwHandles, rwHandles.i64);
      that->pathName = NULL;
   } else {
      /*
       * Named
       */

      that->pathName = Util_SafeStrdup(path);
   }

   that->initialized = TRUE;
   return TRUE;
}


/*
 *----------------------------------------------------------------------
 *
 * SyncWaitQ_Destroy --
 *
 *      Destroys the system resources associated with the specified
 *      waitqueue. The waitqueue structure itself is not freed.
 *
 * Side effects:
 *      Closes handles. On Posix, also unlinks the fifo associated
 *      with this waitqueue.
 *
 *---------------------------------------------------------------------- 
 */

void
SyncWaitQ_Destroy(SyncWaitQ *that)
{
   if (!that->initialized) {
      return;
   }

   if (that->pathName == NULL) {
      /*
       * Anonymous
       */
      
      HandlesAsI64 rwHandles;

      rwHandles.i64 = Atomic_Read64(&that->rwHandles);
      close(rwHandles.fd[0]);
      close(rwHandles.fd[1]);
#if __APPLE__
      ASSERT(Atomic_ReadInt(&workaround) != WORKAROUND_UNKNOWN);
      if (Atomic_ReadInt(&workaround) == WORKAROUND_YES) {
         int result;

         result = pthread_mutex_destroy(&that->mutex);
         ASSERT(!result);
      }
#endif
   } else {
      /*
       * Named
       */
      uint64 seq;
      char *name;

      seq = Atomic_Read64(&that->seq);
      name = SyncWaitQMakeName(that->pathName, seq);
      Posix_Unlink(name);
      free(name);
      free(that->pathName);
      that->pathName = NULL;
   }
   
   that->initialized = FALSE;
}


/*
 *-----------------------------------------------------------------------------
 *
 * SyncWaitQ_Add --
 *
 *      Add a waiter to the waitqueue.
 *
 * Results:
 *      On success, a pollable handle (fd on Posix and a HANDLE on Win32) that
 *         can be used by the caller to determine when the queue has been woken
 *         up.
 *      On failure, -1.
 *
 *-----------------------------------------------------------------------------
 */

PollDevHandle
SyncWaitQ_Add(SyncWaitQ *that) // IN
{
   uint64 seq;
   int ret = -1;
   char *name = NULL;

   ASSERT(that);
   ASSERT(that->initialized);

   // Hint that we are about to wait
   Atomic_Write(&that->waiters, TRUE);
   
   // The following statement is the demarcation line for Add.  Any
   // wakeup that happens after this line should wake up this waiter
   // -- Ticho
   seq = Atomic_Read64(&that->seq);

   /*
    * It is OK to fail in the following 2 paths, because if the sequence number
    * has changed, we manufacture our own fd, so any error is harmless.
    */

   if (that->pathName == NULL) {
      /*
       * Anonymous
       */

      HandlesAsI64 rwHandles;

      /*
       * XXX?
       *
       * There is an extremely small chance for a side effect to
       * unrelated code caused by a race condition here. Please refer
       * to the corresponding comment in syncWaitQWin32.c for more
       * information
       *
       * -- Ticho 
       */

      rwHandles.i64 = Atomic_Read64(&that->rwHandles);
#if __APPLE__
      SyncWaitQLock(that);
#endif
      ret = dup(rwHandles.fd[0]);
#if __APPLE__
      SyncWaitQUnlock(that);
#endif
      if (ret < 0) {
         SyncWaitQPanicOnFdLimit(errno);
      }
   } else {
      /*
       * Named
       */

      name = SyncWaitQMakeName(that->pathName, seq);

      /*
       * Create fifo object with the generated name
       *
       * XXX? Note that if the object already exists on the file
       * system, we will assume that this is a fifo created by another
       * waiter for this wait queue. Of course, this cannot be
       * guaranteed 100%, but in practice this will be the case since
       * the names of the waitqueues are fairly unique and since in
       * practice they will be created in a specially designated
       * directory. -- Ticho.
       */

      ret = Posix_Mkfifo(name, S_IRUSR | S_IWUSR);
      if (ret >= 0 || errno == EEXIST) {
         /*
          * We open in non-blocking mode so that we won't block if nobody
          * has opened in write mode. We prefer to block in poll, because
          * we can block on several wait queues, and we can have a
          * timeout without using signals --hpreg 
          */

         /*
          * It is possible that from the time we create the fifo to the
          * time it is opened, the wait queue was woken up and the fifo
          * was unlinked from the file system. That is OK because we
          * detect this case and will not use the fd returned from open() 
          * -- Ticho.
          */

         ret = Posix_Open(name, O_RDONLY | O_NONBLOCK);
         if (ret < 0) {
            SyncWaitQPanicOnFdLimit(errno);
         }
      }
   }

   // Check to see whether someone didn't wake us up while we were
   // adding ourselves to the queue
   if (seq != Atomic_Read64(&that->seq)) {
      // Someone woke up the queue while we were adding ourselves to it, so
      // just pretend that we were woken up too by returning a conjured up,
      // woken up handle
      int fd[2];

      if (ret >= 0) {
	 close(ret);
         if (that->pathName != NULL) {
            Posix_Unlink(name);
         }
      }

      if (pipe(fd) < 0) {
         SyncWaitQPanicOnFdLimit(errno);
         free(name);
	 return -1;
      }

      if (fcntl(fd[0], F_SETFL, O_RDONLY | O_NONBLOCK) < 0 ||
	  fcntl(fd[1], F_SETFL, O_WRONLY | O_NONBLOCK) < 0 ) {
	 close(fd[0]);
	 close(fd[1]);
         free(name);
	 return -1;
      }
      
      if (write(fd[1], "X", 1) == 1) {
         /*
          * fd[0] now should be in a perpetual woken up state. It will be
          * closed when the client code calls SyncWaitQ_Remove() on it.
          */
	 ret = fd[0];
      } else {
         close(fd[0]);
         ret = -1;
      }

      close(fd[1]);
   } else {
      if (ret < 0) {
         free(name);
         return -1;
      }

      /*
       * It is possible that another thread woke up the queue and set
       * waiters=FALSE, even if we didn't detect that the sequence
       * number has changed. This can happen like so 
       *
       *(T1=this thread, T2=wakeup thread):
       * 
       * T1: Set waiters=TRUE
       * T2: Set waiters=FALSE
       * T2: Create and save new wakeup handle
       * T1: Read seq
       * T1: Dup new wakeup handle
       * T1: compare seq (they match, so the thread won't wake itself up)
       *
       * At this point T1 has a new event handle that hasn't been
       * woken up, but 'waiters' is FALSE. To prevent that case, we
       * set waiters=TRUE once again here. 
       *
       * Note that there is no harm, as far as correctness is
       * concerned in setting waiters=TRUE anywhere
       *
       *      -- Ticho 09/11/2003
       *
       */

      Atomic_Write(&that->waiters, TRUE);
   }

   free(name);

   return ret;
}


/*
 *----------------------------------------------------------------------
 *
 * SyncWaitQ_Remove --
 *
 *      Removes the caller from the waitqueue. The caller must provide
 *      the handle which it originally obtained via a call to
 *      SyncWaitQ_Add
 *
 * Results:
 *      TRUE on success and FALSE otherwise
 *
 * Side effects:
 *      The caller supplied handle is closed
 *
 *---------------------------------------------------------------------- 
 */

Bool
SyncWaitQ_Remove(SyncWaitQ *that,       // Unused
		 PollDevHandle handle)  // IN
{
   ASSERT(that);
   if (!that->initialized) {
      return FALSE;
   }

   /*
    * The anonymous and named case are the same. -- Ticho
    */

   return close(handle) >= 0;
}


/*
 *-----------------------------------------------------------------------------
 *
 * SyncWaitQ_WakeUp --
 *
 *      Wakes up all waiters (if any) by making their pollable handles
 *      signalled.
 *
 *      Note that there are no provisions for the handles previously
 *      given to the waiters to be made unsignalled and used again for
 *      waiting. If an ex-waiter wants to wait again, it will have to
 *      first call SyncWaitQ_Remove and then SyncWaitQ_Add again. The
 *      latter will give it a fresh pollable handle to wait on.
 *
 * Results:
 *      TRUE on success
 *      FALSE on failure
 *
 * Side effects:
 *      None
 *
 *---------------------------------------------------------------------------- 
 */

Bool
SyncWaitQ_WakeUp(SyncWaitQ *that) // IN
{
   ASSERT(that);
   ASSERT(that->initialized);

   if (!Atomic_Read(&that->waiters)) {
      /* Fast path --hpreg */
      return TRUE;
   }

   Atomic_Write(&that->waiters, FALSE);

   /* 
    * Slow path --hpreg 
    */
   
   return (that->pathName == NULL) ? SyncWaitQWakeUpAnon(that) :
                                     SyncWaitQWakeUpNamed(that);
}


/*
 *-----------------------------------------------------------------------------
 *
 * SyncWaitQWakeUpAnon --
 *
 *      Anonymous version of SyncWaitQ_WakeUp
 *
 * Results:
 *      TRUE on success
 *      FALSE on failure
 *
 * Side effects:
 *      None
 *
 *---------------------------------------------------------------------------- 
 */

static Bool
SyncWaitQWakeUpAnon(SyncWaitQ *that)    // IN
{
   HandlesAsI64 rwHandles, wakeupHandles;
   int ret;
   int error;

   // Create the new anonymous handle
   if (pipe(rwHandles.fd) < 0) {
      SyncWaitQPanicOnFdLimit(errno);
      return FALSE;
   }

   if (fcntl(rwHandles.fd[0], F_SETFL, O_RDONLY | O_NONBLOCK) < 0 ||
       fcntl(rwHandles.fd[1], F_SETFL, O_WRONLY | O_NONBLOCK) < 0 ) {
      Warning("SyncWaitQWakeupAnon: fcntl failed, errno = %d\n", errno);
      close(rwHandles.fd[0]);
      close(rwHandles.fd[1]);
      return FALSE;
   }

   // The following statement is the demarcation line for wakeup
   //
   // There is a possibility for suprious wakeups if the WaitQ_Add
   // began executing after this line but before the inc of the sequence.
   //
   // We assume that spurious wakups are OK.
   wakeupHandles.i64 = Atomic_ReadWrite64(&that->rwHandles, rwHandles.i64);
   Atomic_FetchAndInc64(&that->seq);

   ret = write(wakeupHandles.fd[1], "X", 1);
   error = errno;
   close(wakeupHandles.fd[1]);
#if __APPLE__
   SyncWaitQLock(that);
#endif
   close(wakeupHandles.fd[0]);
#if __APPLE__
   SyncWaitQUnlock(that);
#endif
   if (ret != 1) {
      Warning("SyncWaitQWakeupAnon: write failed, ret = %d, errno = %d\n",
              ret, error);
      return FALSE;
   }

   return TRUE;
}


/*
 *-----------------------------------------------------------------------------
 *
 * SyncWaitQWakeUpNamed --
 *
 *      Named version of SyncWaitQ_WakeUp
 *
 * Results:
 *      TRUE on success
 *      FALSE on failure
 *
 * Side effects:
 *      None
 *
 *---------------------------------------------------------------------------- 
 */

static Bool
SyncWaitQWakeUpNamed(SyncWaitQ *that)   // IN
{
   uint64 seq;
   char *name;
   int wakeupHandle = -1;
   int ret;
   int error;

   // The following statement is the demarcation line for wakeup
   seq = Atomic_FetchAndInc64(&that->seq);
   name = SyncWaitQMakeName(that->pathName, seq);

   /*
    * We open in non-blocking mode so that we won't block if there is
    * no reader, instead we will get ENXIO. Another way would be to
    * open in non-blocking read-write mode, but it adds an extra close
    * system call.
    */

   wakeupHandle = Posix_Open(name, O_WRONLY | O_NONBLOCK);
   error = errno;
   Posix_Unlink(name);
   free(name);

   if (wakeupHandle < 0) {
      SyncWaitQPanicOnFdLimit(error);

      /*
       * If error == ENXIO or ENOENT, then there are no waiters, so
       * the wakeup is considered successful.
       */
      if (error == ENXIO || error == ENOENT) {
         return TRUE;
      }

      Warning("SyncWaitQWakeUpNamed: open failed, errno = %d\n", error);
      return FALSE;
   }

   ret = write(wakeupHandle, "X", 1);
   error = errno;
   close(wakeupHandle);
   if (ret != 1) {
      if (ret < 0 && error == EPIPE) {
         /*
          * If a waiter was signalled by another thread and the waiter
          * just closed the read end of the pipe, we may get an EPIPE
          * error. That's OK, because the waiter was already woken up.
          */
         return TRUE;
      }

      Warning("SyncWaitQWakeUpNamed: write failed, ret = %d, errno = %d\n",
              ret, error);
      return FALSE;
   }

   return TRUE;
}


/*
 *----------------------------------------------------------------------
 *
 * SyncWaitQMakeName --
 *
 *       Computes the name of the named system object based on the
 *       path of the wait queue and a sequence number
 *
 *----------------------------------------------------------------------
 */

static char *
SyncWaitQMakeName(const char *path, // IN
                  uint64 seq)       // IN
{
   return Str_SafeAsprintf(NULL, "%s.%"FMT64"x", path, seq);
}
