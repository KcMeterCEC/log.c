/*
 * Copyright (c) 2020 rxi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "log.h"

#include <stdlib.h>
#include <assert.h>

#define MAX_CALLBACKS       32
#define POS_FILE_NAME_LEN   (50)

typedef struct {
    log_LogFn fn;
    void *udata;
    int level;
    char *pos_path;
} Callback;

static struct {
    bool isOpen;
    void *udata;
    log_LockFn lock;
    int level;
    bool quiet;
    Callback callbacks[MAX_CALLBACKS];
    uint32_t file_limit;
} L;


static const char *level_strings[] = {
    "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

#ifdef LOG_USE_COLOR
static const char *level_colors[] = {
    "\x1b[94m", "\x1b[36m", "\x1b[32m", "\x1b[33m", "\x1b[31m", "\x1b[35m"
};
#endif


static void stdout_callback(log_Event *ev) {
    char buf[16];
    buf[strftime(buf, sizeof(buf), "%H:%M:%S", ev->time)] = '\0';
#ifdef LOG_USE_COLOR
    fprintf(
    ev->udata, "%s %s%-5s\x1b[0m \x1b[90m%s:%d:\x1b[0m ",
    buf, level_colors[ev->level], level_strings[ev->level],
    ev->file, ev->line);
#else
    fprintf(
    ev->udata, "%s %-5s %s:%d: ",
    buf, level_strings[ev->level], ev->file, ev->line);
#endif
    vfprintf(ev->udata, ev->fmt, ev->ap);
    fprintf(ev->udata, "\n");
    fflush(ev->udata);
}


static void file_callback(log_Event *ev) {
    char buf[64];

    if((ev->limit) && (ftell(ev->udata) > ev->limit))
    {
        rewind(ev->udata);
    }

  buf[strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", ev->time)] = '\0';
  fprintf(
    ev->udata, "%s %-5s %s:%d: ",
    buf, level_strings[ev->level], ev->file, ev->line);
    vfprintf(ev->udata, ev->fmt, ev->ap);
    fprintf(ev->udata, "\n");
    fflush(ev->udata);
}


static void lock(void)   {
    if (L.lock) { L.lock(true, L.udata); }
}


static void unlock(void) {
    if (L.lock) { L.lock(false, L.udata); }
}


void log_set_lock(log_LockFn fn, void *udata) {
    L.lock = fn;
    L.udata = udata;
}


void log_set_level(int level) {
    L.level = level;
}


void log_set_quiet(bool enable) {
    L.quiet = enable;
}


int log_add_callback(log_LogFn fn, void *udata, int level, char *pos) {
    for (int i = 0; i < MAX_CALLBACKS; i++) {
        if (!L.callbacks[i].fn) {
            L.callbacks[i] = (Callback) { fn, udata, level , pos};
            return 0;
        }
    }
    return -1;
}


int log_add_fp(const char *path, int level) {

    FILE *fp = fopen(path, "r+");
    if(!fp)
    {
        fp = fopen(path, "w");
        if(!fp)
        {
            printf("can't open file: %s\n", path);
            return -1;
        }
    }
    //get the position of log which it was saved last time
    long off = 0;
    char *pos_buf = (char *)malloc(POS_FILE_NAME_LEN);
    assert(pos_buf);

    snprintf(pos_buf, POS_FILE_NAME_LEN, "%s.pos", path);
    FILE *fpos = fopen(pos_buf, "r");
    if(fpos)
    {
        fread(&off, sizeof(off), 1, fpos);
        fclose(fpos);
    }

    fseek(fp, off, SEEK_CUR);

    return log_add_callback(file_callback, fp, level, pos_buf);
}


static void init_event(log_Event *ev, void *udata) {
    if (!ev->time) {
        time_t t = time(NULL);
        ev->time = localtime(&t);
    }
    ev->udata = udata;
}


void log_log(int level, const char *file, int line, const char *fmt, ...) {
    log_Event ev = {
        .fmt   = fmt,
        .file  = file,
        .line  = line,
        .level = level,
    };

    lock();
    if(L.isOpen)
    {
        if (!L.quiet && level >= L.level) {
            init_event(&ev, stderr);
            va_start(ev.ap, fmt);
            stdout_callback(&ev);
            va_end(ev.ap);
        }

        for (int i = 0; i < MAX_CALLBACKS && L.callbacks[i].fn; i++) {
            Callback *cb = &L.callbacks[i];
            if (level >= cb->level) {
                init_event(&ev, cb->udata);
                va_start(ev.ap, fmt);
                ev.limit = L.file_limit;
                cb->fn(&ev);
                va_end(ev.ap);
            }
        }
    }

    unlock();
}

void log_set_file_limit(uint32_t limit)
{
    L.file_limit = limit;
}

void log_close(void)
{
    lock();

    for (int i = 0; i < MAX_CALLBACKS && L.callbacks[i].fn; i++) {
        Callback *cb = &L.callbacks[i];
        FILE *fp = (FILE *)cb->udata;
        long off = ftell(fp);
        fclose(fp);

        fp = fopen(cb->pos_path, "w");
        if(fp)
        {
        fwrite(&off, sizeof(off), 1, fp);
        fclose(fp);
        }
        else
        {
            printf("error: Can't open file: %s\n", cb->pos_path);
        }
        if(cb->pos_path)
        {
            free(cb->pos_path);
        }
    }
    L.isOpen = false;

    unlock();
}
void log_open(void)
{
    L.isOpen = true;
}
