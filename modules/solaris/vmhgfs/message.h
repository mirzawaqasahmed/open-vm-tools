/*********************************************************
 * Copyright (C) 1999 VMware, Inc. All rights reserved.
 *
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
 * message.h --
 *
 *    Second layer of the internal communication channel between guest
 *    applications and vmware
 */

#ifndef __MESSAGE_H__
#   define __MESSAGE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "vm_basic_types.h"


typedef struct Message_Channel Message_Channel;


/*
 * These functions must be implemented by any external Message
 * transport implementation. Some examples include crossTalk,
 * a network socket, or a Microsoft Hypervisor backdoor.
 *
 * These external functions mirror the same corresponding Message_* 
 * functions below.
 */
typedef Message_Channel *(*MessageOpenProcType)(uint32 proto);

typedef Bool (*MessageGetReadEventProcType)(Message_Channel *chan,
                                            int64 *readEvent);

typedef Bool (*MessageSendProcType)(Message_Channel *chan,
                                    const unsigned char *buf,
                                    size_t bufSize);
typedef Bool (*MessageReceiveProcType)(Message_Channel *chan,
                                       unsigned char **buf,
                                       size_t *bufSize);
typedef Bool (*MessageCloseProcType)(Message_Channel *chan);


/*
 * This tells the message layer to use an alternate transport
 * for messages. By default, we use the backdoor, so this function
 * overrides that default at runtime and switches everything over to
 * an alternate transport.
 */
void Message_SetTransport(MessageOpenProcType openProc,
                          MessageGetReadEventProcType getReadEeventProc,
                          MessageSendProcType sendProc,
                          MessageReceiveProcType receiveProc,
                          MessageCloseProcType closeProc);

void MessageStub_RegisterTransport(void);

Message_Channel *
Message_Open(uint32 proto); // IN

/*
 * This allows higher levels of the IPC stack to use an event to detect
 * when a message has arrived. This allows an interrupt-model rather than
 * continually calling Message_Receive in a busy loop. This may only be supported
 * by some transports. The backdoor does not, so the IPC code will still
 * have to poll in those cases.
 */
Bool
Message_GetReadEvent(Message_Channel *chan,    // IN
                     int64 *event);            // OUT

Bool
Message_Send(Message_Channel *chan,    // IN/OUT
             const unsigned char *buf, // IN
             size_t bufSize);          // IN

Bool
Message_Receive(Message_Channel *chan, // IN/OUT
                unsigned char **buf,   // OUT
                size_t *bufSize);      // OUT

Bool
Message_Close(Message_Channel *chan); // IN/OUT

#ifdef __cplusplus
}
#endif

#endif /* __MESSAGE_H__ */
