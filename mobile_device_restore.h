#ifndef MOBILE_DEVICE_RESTORE_H
#define MOBILE_DEVICE_RESTORE_H

#include <CoreFoundation/CoreFoundation.h>

#include "MobileDevice.h"

void mobile_device_initiate_restore(CFStringRef deviceId, service_conn_t connection, CFMutableDictionaryRef files);
void *mobile_device_add_restore_file(CFMutableDictionaryRef files, CFDataRef crap, CFStringRef domain, CFStringRef path, int uid, int gid, int mode);

#endif