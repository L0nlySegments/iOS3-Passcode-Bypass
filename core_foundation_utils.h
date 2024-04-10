#ifndef CORE_FOUNDATION_UTILS_H
#define CORE_FOUNDATION_UTILS_H

#include <CoreFoundation/CoreFoundation.h>

extern void* cf_allocate(uint32_t size);

extern const char* cf_create_c_string(CFStringRef ref);
extern CFStringRef cf_create_string_with_format(CFStringRef format, ...);
extern CFNumberRef cf_create_number_with_int(int number);
extern CFNumberRef cf_create_number_with_cfindex(CFIndex index);

#define cf_create_dict_m(args...) cf_create_dict(sizeof((const void *[]){args}) / sizeof(const void *) / 2, (const void *[]){args})
extern CFMutableDictionaryRef cf_create_dict(int number_of_elements, const void **dict);

#define cf_create_array_m(args...) cf_create_array(sizeof((const void *[]){args}) / sizeof(const void *), (const void *[]){args})
extern CFMutableArrayRef cf_create_array(int number_of_elements, const void **array);

#endif