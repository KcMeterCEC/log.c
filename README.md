# log.c
A simple logging library implemented in C99

![screenshot](https://cloud.githubusercontent.com/assets/3920290/23831970/a2415e96-0723-11e7-9886-f8f5d2de60fe.png)


## Usage
**[log.c](src/log.c?raw=1)** and **[log.h](src/log.h?raw=1)** should be dropped
into an existing project and compiled along with it. The library provides 6
function-like macros for logging:

```c
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
```

Each function takes a printf format string followed by additional arguments:

```c
log_trace("Hello %s", "world")
```

Resulting in a line with the given format printed to stderr:

```
20:18:26 TRACE src/main.c:11: Hello world
```

#### log_open(void) / log_close(void)
`log_open()` should be used first. Then you can use log_xxx function.

`log_close()` should be used when you exit process.


#### log_set_quiet(bool enable)
Quiet-mode can be enabled by passing `true` to the `log_set_quiet()` function.
While this mode is enabled the library will not output anything to `stderr`, but
will continue to write to files if any are set.


#### log_set_level(int level)
The current logging level can be set by using the `log_set_level()` function.
All logs below the given level will not be written to `stderr`. By default the
level is `LOG_TRACE`, such that nothing is ignored.


#### log_add_fp(const char *path, int level)
One or more file paths where the log will be written can be provided to the
library by using the `log_add_fp()` function. The data written to the file
output is of the following format:

```
2047-03-11 20:18:26 TRACE src/main.c:11: Hello world
```

Any messages below the given `level` are ignored. If the library failed to add a
file path a value less-than-zero is returned.


#### log_set_lock(log_LockFn fn, void *udata)
If the log will be written to from multiple threads a lock function can be set.
The function is passed the boolean `true` if the lock should be acquired or
`false` if the lock should be released and the given `udata` value.

#### log_set_file_limit(uint32_t limit)
Set the limit of log file size.

The default value is 0,which means there isn't limit for log file.

If the log string is larger than limit,the log string will be written rollback at the beginning of file.

#### LOG_USE_COLOR
If the library is compiled with `-DLOG_USE_COLOR` ANSI color escape codes will
be used when printing.


## License
This library is free software; you can redistribute it and/or modify it under
the terms of the MIT license. See [LICENSE](LICENSE) for details.
