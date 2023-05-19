CC = clang

.PHONY: all
all: bypass

bypass: bypass.c
	$(CC) -o bypass bypass.c sha1dgst.c -framework CoreFoundation -include MobileDevice.h -include sha.h -include sha_locl.h