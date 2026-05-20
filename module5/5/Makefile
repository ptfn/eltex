obj-m += netlink.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(CURDIR) modules
	gcc test.c -o test

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(CURDIR) clean
	rm -f test
