#include "support.h"
#include <cassert>

#if defined(PLATFORM_LINUX) || defined(PLATFORM_ANDROID)

Thread::Thread() :
	running(false),
	m_pipe_r(NULL),
	m_pipe_w(NULL)
{
	pthread_attr_init(&m_thread_attr);
	int pipe_wr[] = {0, 0};
	pipe(pipe_wr);
	m_pipe_r = fdopen(pipe_wr[0], "r");
	m_pipe_w = fdopen(pipe_wr[1], "w");
}

Thread::~Thread() {
	fclose(m_pipe_r);
	fclose(m_pipe_w);
	pthread_attr_destroy(&m_thread_attr);
}

void Thread::doCleanup(void *data) {
	Thread *thread = (Thread *) data;
	thread->running = false;
}

void *Thread::doRun(void *data) {
	pthread_cleanup_push(&Thread::doCleanup, data);

#if defined(PLATFORM_LINUX)
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
#elif defined(PLATFORM_ANDROID)
    struct sigaction actions;
    memset(&actions, 0, sizeof(actions));
    sigemptyset(&actions.sa_mask);
    actions.sa_flags = 0;
    actions.sa_handler = doSigHandler;
    int rc = sigaction(SIGUSR1, &actions, NULL);
	assert(rc == 0);
#endif

	Thread *thread = (Thread *) data;
	thread->run();

	pthread_cleanup_pop(1);
	return NULL;
}

#if defined(PLATFORM_ANDROID)
void Thread::doSigHandler(int sig) {
    pthread_exit(0);
}
#endif

bool Thread::start() {
	int ret = pthread_create(&m_thread, &m_thread_attr, &Thread::doRun, this);
	running = (ret == 0);
	return running;
}

bool Thread::cancel() {
#if defined(PLATFORM_LINUX)
	int ret = pthread_cancel(m_thread);
#elif defined(PLATFORM_ANDROID)
    int ret = pthread_kill(m_thread, SIGUSR1);
#endif
	running = (ret != 0);
	return !running;
}

#elif defined(PLATFORM_WIN32)


Thread::Thread() :
	running(false),
	m_thread(NULL),
	m_threadReadyEvent(NULL),
	m_threadID(0)
{
	m_threadReadyEvent = CreateEvent(NULL, false, false, NULL);
}

Thread::~Thread() {
	CloseHandle(m_threadReadyEvent);
}

DWORD WINAPI Thread::doRun(void *data) {
	// create thread message queue
	MSG msg;
	PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

	Thread *thread = (Thread *) data;
	SetEvent(thread->m_threadReadyEvent);
	thread->run();
	return 0;
}

bool Thread::start() {
	m_thread = CreateThread(NULL, 0, Thread::doRun, this, 0, &m_threadID);
	if (m_thread == NULL) return false;
	WaitForSingleObject(m_threadReadyEvent, INFINITE);
	running = (m_thread != NULL);
	return running;
}

bool Thread::cancel() {
	// FIXME: this is dangerous
	int ret = TerminateThread(m_thread, 0);
	if (ret == 0) return false;
	CloseHandle(m_thread);
	m_thread = NULL;
	m_threadID = 0;
	running = false;
	return true;
}

#endif
