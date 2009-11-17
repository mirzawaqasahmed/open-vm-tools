/*********************************************************
 * Copyright (C) 2002 VMware, Inc. All rights reserved.
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
 * conf.h --
 *
 *    Manage the tools configuration file.
 *
 */


#ifndef __CONF_H__
#define __CONF_H__

#include "guestApp.h"

#define CONF_FILE         "tools.conf"

#if ! defined(_WIN32)
#   define CONFVAL_POWERONSCRIPT_DEFAULT  "poweron-vm-default"
#   define CONFVAL_POWEROFFSCRIPT_DEFAULT "poweroff-vm-default"
#   define CONFVAL_RESUMESCRIPT_DEFAULT   "resume-vm-default"
#   define CONFVAL_SUSPENDSCRIPT_DEFAULT  "suspend-vm-default"
#else
#   define CONFVAL_POWERONSCRIPT_DEFAULT  "poweron-vm-default.bat"
#   define CONFVAL_POWEROFFSCRIPT_DEFAULT "poweroff-vm-default.bat"
#   define CONFVAL_RESUMESCRIPT_DEFAULT   "resume-vm-default.bat"
#   define CONFVAL_SUSPENDSCRIPT_DEFAULT  "suspend-vm-default.bat"
#endif

#define CONFNAME_MAX_WIPERSIZE            "max.wiperfile.size"
#define CONFNAME_POWERONSCRIPT            "poweron-script"
#define CONFNAME_POWEROFFSCRIPT           "poweroff-script"
#define CONFNAME_RESUMESCRIPT             "resume-script"
#define CONFNAME_SUSPENDSCRIPT            "suspend-script"
#define CONFNAME_LOG                      "log"
#define CONFNAME_LOGFILE                  "log.file"
#define CONFNAME_LOGLEVEL                 "log.level" 
#define CONFNAME_DISABLETOOLSVERSION      "disable-tools-version"
#define CONFNAME_DISABLEPMTIMERWARNING    "disable-pmtimerwarning"


/*
 ******************************************************************************
 * BEGIN GuestInfo goodies.
 */

/**
 * Defines the string used for the GuestInfo config file group.
 */
#define CONFGROUPNAME_GUESTINFO "guestinfo"

/**
 * Lets users disable just DiskInfo.
 */
#define CONFNAME_GUESTINFO_DISABLEQUERYDISKINFO "disable-query-diskinfo"

/**
 * Define a custom GuestInfo poll interval (in seconds).
 *
 * @note Illegal values result in a @c g_warning and fallback to the default
 * poll interval.
 *
 * @param int   User-defined poll interval.  Set to 0 to disable polling.
 */
#define CONFNAME_GUESTINFO_POLLINTERVAL "poll-interval"

/*
 * END GuestInfo goodies.
 ******************************************************************************
 */


/*
 * Tell the tools to show the wireless icon in the guest.
 */

#define CONFNAME_SHOW_WIRELESS_ICON "wirelessIcon.enable"

/*
 * Directory containing the tools library files.  Currently only intended
 * for vmware-user.
 */
#if !defined(_WIN32)
#   define CONFNAME_LIBDIR                        "libdir"
#endif

/* Default maximum size of wiper file in MB */
#define CONFVAL_MAX_WIPERSIZE_DEFAULT         "512"

/* Wait 5 seconds between polls to see if the conf file has changed */
#define CONF_POLL_TIME     500

GuestApp_Dict *Conf_Load(void);
Bool Conf_ReloadFile(GuestApp_Dict **pConfDict);

#endif /* __CONF_H__ */
