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


struct qalc_lconv_t {
	bool int_p_cs_precedes;
	bool int_n_cs_precedes;
	bool p_cs_precedes;
	bool n_cs_precedes;
	string thousands_sep;
	string decimal_point;
};

#if defined(PLATFORM_ANDROID)
// TODO: where do I put this stuff?
struct AndroidContext {
	string internalDir;
	string externalDir;
	qalc_lconv_t lconv;
};
// both instantiated in qalculate-android/jni/main.cpp
extern AndroidContext gAndroidContext;
extern pthread_mutex_t gAndroidLogMutex;

#include <android/log.h>

#define LOGX(x, ...) do { \
	{ \
		int _logx_ret = pthread_mutex_lock(&gAndroidLogMutex); \
		assert(_logx_ret == 0); \
	} \
	__android_log_print(x, "libqalculate", __VA_ARGS__); \
	{ \
		int _logx_ret = pthread_mutex_unlock(&gAndroidLogMutex); \
		assert(_logx_ret == 0); \
	} \
} while (0)
#define LOGD(...) LOGX(ANDROID_LOG_DEBUG, __VA_ARGS__)
#define LOGW(...) LOGX(ANDROID_LOG_WARN, __VA_ARGS__)
#define LOGE(...) LOGX(ANDROID_LOG_ERROR, __VA_ARGS__)

#else
#define LOGD(...)
#define LOGW(...)
#define LOGE(...)

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
	virtual const char *TAG() = 0;

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

#endif
