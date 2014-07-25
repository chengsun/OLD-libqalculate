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

class Thread {
public:
	Thread();
	virtual ~Thread();
	bool start();
	bool cancel();
	template <class T> bool write(T data) {
#if defined(PLATFORM_LINUX) || defined(PLATFORM_ANDROID)
		fwrite(&data, sizeof(T), 1, m_pipe_w);
		fflush(m_pipe_w);
		return true;

#elif defined(PLATFORM_WIN32)
		int ret = PostThreadMessage(m_threadID, WM_USER, (WPARAM) data, 0);
		return (ret != 0);
#endif
	}

	// FIXME: this is technically wrong -- needs memory barriers (std::atomic?)
	volatile bool running;

protected:
	virtual void run() = 0;
	template <class T> T read() {
#if defined(PLATFORM_LINUX) || defined(PLATFORM_ANDROID)
		T x;
		fread(&x, sizeof(T), 1, m_pipe_r);
		return x;

#elif defined(PLATFORM_WIN32)
		MSG msg;
		int ret = GetMessage(&msg, NULL, WM_USER, WM_USER);
		return (T) msg.wParam;
#endif
	}

private:
#if defined(PLATFORM_LINUX) || defined(PLATFORM_ANDROID)
	static void doCleanup(void *data);
	static void *doRun(void *data);
#if defined(PLATFORM_ANDROID)
	static void doSigHandler(int sig);
#endif

	pthread_t m_thread;
	pthread_attr_t m_thread_attr;
	FILE *m_pipe_r, *m_pipe_w;

#elif defined(PLATFORM_WIN32)
	static DWORD WINAPI doRun(void *data);

	HANDLE m_thread, m_threadReadyEvent;
	DWORD m_threadID;
#endif
};


#include <string>

struct qalc_lconv_t {
	bool int_p_cs_precedes;
	bool int_n_cs_precedes;
	bool p_cs_precedes;
	bool n_cs_precedes;
	std::string thousands_sep;
	std::string decimal_point;
};

#if defined(PLATFORM_ANDROID)
struct AndroidContext {
	std::string internalDir;
	qalc_lconv_t lconv;
};
extern AndroidContext gAndroidContext;
#endif

#endif
