#include <stdio.h>
#include <stdlib.h>
#include <CoreFoundation/CoreFoundation.h>

#include "sha.h"
#include "MobileDevice.h"
#include "log_utils.h"
#include "core_foundation_utils.h"
#include "mobile_device_communication.h"

//==== Data Helpers ====
static CFStringRef data_to_hex(CFDataRef input) {
    CFMutableStringRef result = CFStringCreateMutable(NULL, 40);
    const unsigned char *bytes = CFDataGetBytePtr(input);
    unsigned int len = CFDataGetLength(input);
    unsigned int i;
    for(i = 0; i < len; i++) {
        CFStringAppendFormat(result, NULL, CFSTR("%02x"), (unsigned int) bytes[i]);
    }
    return result;
}

static CFDataRef sha1_of_data(CFDataRef input) {
    unsigned char *md = cf_allocate(20);
    SHA_CTX ctx;
    SHA1_Init(&ctx);
    SHA1_Update(&ctx, CFDataGetBytePtr(input), CFDataGetLength(input));
    SHA1_Final(md, &ctx);
    return CFDataCreateWithBytesNoCopy(NULL, md, 20, NULL);
}

void mobile_device_initiate_restore(CFStringRef deviceId, service_conn_t connection, CFMutableDictionaryRef files) {

    CFMutableDictionaryRef m1dict = cf_create_dict_m(
        CFSTR("6.2"), 							CFSTR("Version"),
        deviceId, 								CFSTR("DeviceId"),
        cf_create_dict_m(), 							CFSTR("Applications"),
        files, 									CFSTR("Files")
    );
    CFDataRef m1data = CFPropertyListCreateXMLData(NULL, m1dict);

    CFMutableDictionaryRef manifest = cf_create_dict_m(
        CFSTR("2.0"),							CFSTR("AuthVersion"),
        sha1_of_data(m1data), 					CFSTR("AuthSignature"),
        cf_create_number_with_int(0), 			CFSTR("IsEncrypted"),
        m1data, 								CFSTR("Data")
    );

    CFMutableDictionaryRef mdict = cf_create_dict_m(
        CFSTR("kBackupMessageRestoreRequest"), 	CFSTR("BackupMessageTypeKey"),
        kCFBooleanTrue,						CFSTR("BackupNotifySpringBoard"),
        kCFBooleanTrue, 						CFSTR("BackupPreserveCameraRoll"),
        CFSTR("3.0"), 							CFSTR("BackupProtocolVersion"),
        manifest, 								CFSTR("BackupManifestKey")
    );

    mobile_device_send_msg(connection, cf_create_array_m(CFSTR("DLMessageProcessMessage"), mdict));
}

void *mobile_device_add_restore_file(CFMutableDictionaryRef files, CFDataRef crap, CFStringRef domain, CFStringRef path, int uid, int gid, int mode) {
    CFDataRef pathdata = CFStringCreateExternalRepresentation(NULL, path, kCFStringEncodingUTF8, 0);
    CFStringRef manifestkey = data_to_hex(sha1_of_data(pathdata));
    CFMutableDictionaryRef dict = cf_create_dict_m(
        domain, CFSTR("Domain"),
        path, CFSTR("Path"),
        kCFBooleanFalse, CFSTR("Greylist"),
        CFSTR("3.0"), CFSTR("Version"),
        crap, CFSTR("Data")
    );

    unsigned char *datahash = cf_allocate(20);
    const char *extra = ";(null);(null);(null);3.0";
    SHA_CTX ctx;
    SHA1_Init(&ctx);
    SHA1_Update(&ctx, CFDataGetBytePtr(crap), CFDataGetLength(crap));
    SHA1_Update(&ctx, CFDataGetBytePtr(pathdata), CFDataGetLength(pathdata));
    SHA1_Update(&ctx, (const unsigned char *)extra, strlen(extra));
    SHA1_Final(datahash, &ctx);

    CFDictionarySetValue(files,
        manifestkey,
        cf_create_dict_m(
            CFDataCreateWithBytesNoCopy(NULL, datahash, 20, kCFAllocatorNull), CFSTR("DataHash"),
            domain, CFSTR("Domain"),
            cf_create_number_with_cfindex(CFDataGetLength(crap)), CFSTR("FileLength"),
            cf_create_number_with_int(gid), CFSTR("Group ID"),
            cf_create_number_with_int(uid), CFSTR("User ID"),
            cf_create_number_with_int(mode), CFSTR("Mode"),
            CFDateCreate(NULL, 2020964986UL), CFSTR("ModificationTime")
        ));
    
    CFStringRef templateo = CFStringCreateWithFormat(NULL, NULL, CFSTR("/tmp/stuff.%06d"), arc4random());
    CFMutableDictionaryRef info = cf_create_dict_m(
        templateo, CFSTR("DLFileDest"),
        path, CFSTR("Path"),
        CFSTR("3.0"), CFSTR("Version"),
        crap, CFSTR("Crap")
    );

    return info;
}



