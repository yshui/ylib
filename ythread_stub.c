#include <windows.h>
#include <process.h>
#include <sys/timeb.h>
#include <stdlib.h>
#define TSS_DTOR_ITERATIONS (4)
#define NULL ((void *)(0))
typedef DWORD tss_t;
struct YThreadTSSData {
	void* value;
	tss_t key;
	struct YThreadTSSData* next;
};

tss_dtor_t _ythread_tss_dtors[1088] = { NULL, };

_Thread_local struct YThreadTSSData* _ythread_tss_head = NULL;
_Thread_local struct YThreadTSSData* _ythread_tss_tail = NULL;

static void _ythread_tss_cleanup () {
	struct YThreadTSSData* data;
	int iteration;
	unsigned int again = 1;
	void* value;

	for (iteration = 0 ; iteration < TSS_DTOR_ITERATIONS && again > 0 ; iteration++)
	{
		again = 0;
		for (data = _ythread_tss_head ; data != NULL ; data = data->next)
		{
			if (data->value != NULL)
			{
				value = data->value;
				data->value = NULL;

				if (_ythread_tss_dtors[data->key] != NULL)
				{
					again = 1;
					_ythread_tss_dtors[data->key](value);
				}
			}
		}
	}

	while (_ythread_tss_head != NULL) {
		data = _ythread_tss_head->next;
		free (_ythread_tss_head);
		_ythread_tss_head = data;
	}
	_ythread_tss_head = NULL;
	_ythread_tss_tail = NULL;
}

static void NTAPI _ythread_tss_callback(PVOID h, DWORD dwReason, PVOID pv)
{
	(void)h;
	(void)pv;

	if (_ythread_tss_head != NULL && (dwReason == DLL_THREAD_DETACH || dwReason == DLL_PROCESS_DETACH))
		_ythread_tss_cleanup();
}

#if defined(_MSC_VER)
#pragma data_seg(".CRT$XLB")
PIMAGE_TLS_CALLBACK p_thread_callback = _ythread_tss_callback;
#pragma data_seg()
#else
PIMAGE_TLS_CALLBACK p_thread_callback __attribute__((section(".CRT$XLB"))) = _ythread_tss_callback;
#endif
