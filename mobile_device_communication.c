#include <stdio.h>
#include <sys/socket.h>
#include <CoreFoundation/CoreFoundation.h>
#include <stdlib.h>

#include "sha.h"
#include "MobileDevice.h"
#include "core_foundation_utils.h"

void mobile_device_send_msg(service_conn_t conn, CFPropertyListRef plist) {
    CFDataRef data = CFPropertyListCreateXMLData(NULL, plist);
    uint32_t size = htonl(CFDataGetLength(data));

    send(conn, &size, sizeof(size), 0);
    send(conn, CFDataGetBytePtr(data), CFDataGetLength(data), 0);

    CFRelease(data);
}

CFStringRef mobile_device_receive_msg(service_conn_t conn) {
    uint32_t size;

    ssize_t ret = recv((int) conn, &size, sizeof(size), 0);
    size = ntohl(size);

    void *buf = cf_allocate(size);
    char *p = buf; uint32_t br = size;
    while(br) {
        ret = recv((int) conn, p, br, 0);
        br -= ret;
        p += ret;
    }

    CFDataRef data = CFDataCreateWithBytesNoCopy(NULL, buf, size, NULL);
    CFPropertyListRef result = CFPropertyListCreateFromXMLData(NULL, data, kCFPropertyListImmutable, NULL);
    CFStringRef strResult = cf_create_string_with_format(CFSTR("%@"), result);

    CFRelease(data);
    CFRelease(result);

    return strResult;
}

void mobile_device_send_file(service_conn_t conn, CFMutableDictionaryRef info) {
    CFDictionarySetValue(info, CFSTR("DLFileAttributesKey"), cf_create_dict_m());
    CFDictionarySetValue(info, CFSTR("DLFileSource"), CFDictionaryGetValue(info, CFSTR("DLFileDest")));
    CFDictionarySetValue(info, CFSTR("DLFileIsEncrypted"), cf_create_number_with_int(0));
    CFDictionarySetValue(info, CFSTR("DLFileOffsetKey"), cf_create_number_with_int(0));

    CFDataRef crap = CFDictionaryGetValue(info, CFSTR("Crap"));
    CFDictionaryRemoveValue(info, CFSTR("Crap"));
    CFDictionarySetValue(info, CFSTR("DLFileStatusKey"), cf_create_number_with_int(2));
    mobile_device_send_msg(conn, cf_create_array_m(CFSTR("DLSendFile"), crap, info));
}

