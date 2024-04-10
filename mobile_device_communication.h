#ifndef MOBILE_DEVICE_COMMUNICATION
#define MOBILE_DEVICE_COMMUNICATION

#include <stdio.h>
#include <sys/socket.h>
#include <CoreFoundation/CoreFoundation.h>
#include "sha.h"
#include "MobileDevice.h"

void mobile_device_send_msg(service_conn_t conn, CFPropertyListRef plist);
CFStringRef mobile_device_receive_msg(service_conn_t conn);

void mobile_device_send_file(service_conn_t conn, CFMutableDictionaryRef info);

#endif