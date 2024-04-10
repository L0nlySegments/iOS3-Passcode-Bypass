#include <stdio.h>
#include <CoreFoundation/CoreFoundation.h>

#include "core_foundation_utils.h"

static const char* getEventName(t_log_event event) {
    switch(event) {
        case INFO: return "INFO";
        case RESTORE: return "RESTORE";
        case ERROR: return "ERROR"; 
    }
} 

void log_string_to_stdout(t_log_event event, const char *string) {
    printf("[%s] %s\n", getEventName(event), string);
}

void log_cf_string_to_stdout(t_log_event event, CFStringRef refString) {
    log_string_to_stdout(event, cf_create_c_string(refString));
}