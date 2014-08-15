gcc -g pakfs.c pakstuff.c -lfuse -D_FILE_OFFSET_BITS=64 -o pakmount
