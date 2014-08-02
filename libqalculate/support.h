/*
    Qalculate    

    Copyright (C) 2003  Niklas Knutsson (nq@altern.org)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef SUPPORT_H
#define SUPPORT_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <cassert>
#include <string>
using namespace std;

#if defined(PLATFORM_LINUX) || defined(PLATFORM_ANDROID)
#	include <cstdio>
#	include <pthread.h>
#	include <unistd.h>
#elif defined(PLATFORM_WINDOWS)
#	include <windows.h>
#else
#	error "No thread support available on this platform"
#endif

#ifdef PLATFORM_WIN32
	#define srand48 srand
#endif

/*
 * Standard gettext macros.
 */
#ifdef ENABLE_NLS
#  include <libintl.h>
#  undef _
#  define _(String) dgettext (PACKAGE, String)
#  ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#  else
#    define N_(String) (String)
#  endif
#else
#  define textdomain(String) (String)
#  define gettext(String) (String)
#  define dgettext(Domain,Message) (Message)
#  define dcgettext(Domain,Message,Type) (Message)
#  define bindtextdomain(Domain,Directory) (Domain)
#  define _(String) (String)
#  define N_(String) (String)
#endif

#endif
