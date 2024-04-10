#include <stdio.h>
#include <stdarg.h>
#include <CoreFoundation/CoreFoundation.h>

/* Data utilities */
void* cf_allocate(uint32_t size) {
    return CFAllocatorAllocate(NULL, (size), 0);
}

/* String Utilities  */
const char* cf_create_c_string(CFStringRef ref) {
    return CFStringGetCStringPtr(ref, kCFStringEncodingMacRoman);
} 

CFStringRef cf_create_string_with_format(CFStringRef format, ...) {
    va_list args;
    va_start(args, format);

    CFStringRef result = CFStringCreateWithFormatAndArguments(NULL, NULL,format, args);
    va_end(args);
    
    return result;
}

/* Number Utilities */
CFNumberRef cf_create_number_with_int(int number) {
    return CFNumberCreate(NULL, kCFNumberSInt32Type, (int[]){number});
}

CFNumberRef cf_create_number_with_cfindex(CFIndex index) {
    return CFNumberCreate(NULL, kCFNumberCFIndexType, (CFIndex[]){index});
}

/* Array and Dictionary Utilities */
CFMutableDictionaryRef cf_create_dict(int number_of_elements, const void **dict) {
    CFMutableDictionaryRef ref = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    while(number_of_elements--) {
        const void *value = *dict++;
        const void *key = *dict++;
        CFDictionarySetValue(ref, key, value);
    }
    return ref;
}

CFMutableArrayRef cf_create_array(int number_of_elements, const void **array) {
    CFMutableArrayRef ref = CFArrayCreateMutable(NULL, 0, &kCFTypeArrayCallBacks);
    while(number_of_elements--) {
        CFArrayAppendValue(ref, *array++);
    }
    return ref;
}