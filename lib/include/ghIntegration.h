/*********************************************************
 * Copyright (C) 2007 VMware, Inc. All rights reserved.
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

/*
 * ghIntegration.h --
 *
 *    Commands for guest host integration.
 */

#ifndef _GH_INTEGRATION_H_
#define _GH_INTEGRATION_H_

#include "dbllnklst.h"
#include "rpcin.h"


extern DblLnkLst_Links launchMenu;

/*
 * If other libraries want to use dynamic adding/removing of event monitoring
 * to VMWareUserLoop then following definitions need to move to a header
 * file, which is shared by all the libraries. To make such a move simple, following
 * definitions do not have GHI prefix, rather it has VMU (aka VMwareUser)
 * prefix.
 */
#ifdef _WIN32
typedef HANDLE VMU_EVENT;
#else
typedef int VMU_EVENT;
#endif


typedef enum VmuCallbackAction {
   VMU_CALLBACK_ACTION_SUCCESS,
   VMU_CALLBACK_ACTION_ABORT
} VmuCallbackAction;
typedef VmuCallbackAction VMU_EventHandler(void *ctx, VMU_EVENT event);

typedef enum VmuControllerAction {
   VMU_CONTROLLER_CB_ADD_EVENT = 1,
   VMU_CONTROLLER_CB_REMOVE_EVENT
} VmuControllerAction;
typedef Bool (VMU_ControllerCB)(void *ctx,
                                VMU_EVENT event,
                                VMU_EventHandler *eventHandler,
                                void *cbCtx,
                                VmuControllerAction action);

Bool GHI_IsSupported(void);
void GHI_Init(VMU_ControllerCB *vmuControllerCB, void *ctx);
void GHI_Cleanup(void);
void GHI_InitBackdoor(struct RpcIn *rpcIn);
void GHI_RegisterCaps(void);
void GHI_UnregisterCaps(void);
void GHI_Gather(void);

#endif

