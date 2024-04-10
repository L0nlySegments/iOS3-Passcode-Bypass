CC = clang

.PHONY: all
all: bypass

bypass: bypass.c sha1dgst.c core_foundation_utils.c log_utils.c
	$(CC) -o bypass bypass.c sha1dgst.c core_foundation_utils.c log_utils.c mobile_device_communication.c mobile_device_restore.c -framework CoreFoundation -include MobileDevice.h -include sha.h -include sha_locl.h -include mobile_device_restore.h -include core_foundation_utils.h -include log_utils.h