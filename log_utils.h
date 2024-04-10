#ifndef LOG_UTILS_H
#define LOG_UTILS_H

#include <CoreFoundation/CoreFoundation.h>

typedef enum {
    INFO, 
    RESTORE, 
    ERROR
} t_log_event;

extern void log_string_to_stdout(t_log_event event, const char *s);
extern void log_cf_string_to_stdout(t_log_event event, CFStringRef ref);

#endif