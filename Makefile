all: compute_trans file_create

compute_trans: compute_trans.o
compute_trans: LDLIBS = -lselinux

file_create: file_create.o common.o
file_create: LDLIBS = -lselinux

clean:
	$(RM) compute_trans file_create *.o

.PHONY: all clean
