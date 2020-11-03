

#include "log.h"

#include <pthread.h>


pthread_mutex_t     mutex_log;

static void log_lock(bool lock, void *udata)
{
	pthread_mutex_t *mtx = (pthread_mutex_t *)udata;
	if(lock)
    {
        pthread_mutex_lock(mtx);        
    }
    else 
    {
        pthread_mutex_unlock(mtx);       
    }
}

int main(void)
{
    log_open();
	if(pthread_mutex_init(&mutex_log, NULL) != 0)
    {
        log_fatal("mutex init error!\n");
    }

    log_set_lock(log_lock, &mutex_log);

    log_set_quiet(false);
    log_set_level(LOG_TRACE);
    log_set_file_limit(512);

    log_add_fp("./log_out", LOG_TRACE);

	log_trace("This is a log");
	log_debug("This is a log");
	log_info("This is a log");
	log_warn("This is a log");
	log_error("This is a log");
	log_fatal("This is a log");

	log_close();

	return 0;
}