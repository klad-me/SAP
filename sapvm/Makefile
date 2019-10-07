CFLAGS= -Wall -Wshadow -Wstrict-prototypes -I. -O3 -Ipt
LDFLAGS=-lm
CC=gcc


SRC=	main.c				\
	sap_native_File.c		\
	sap_native_Socket.c		\
	sapvm/sapvm.c			\
	sapvm/vm.c			\
	sapvm/sap_native.c		\
	sapvm/sap_native_System.c	\
	sapvm/sap_native_Integer.c	\
	sapvm/sap_native_Float.c	\
	sapvm/sap_native_Math.c		\
	sapvm/sap_native_Thread.c	\
	sapvm/sap_native_IPC.c		\
	sapvm/sap_native_Random.c	\
	sapvm/sap_native_Pack.c		\
	sapvm/sap_native_String.c	\
	sapvm/sap_native_StringBuffer.c	\
	sapvm/sap_native_DateTime.c	\
	sapvm/sap_native_Vector.c	\
	sapvm/sap_native_CRC16.c	\
	sapvm/opcodes.c			\
	sapvm/gc.c


.PHONY:	all

all:	vm

clean:
	rm -f vm $(SRC:.c=.o)

vm:	$(SRC:.c=.o)
	$(CC) $(CFLAGS) -o $@ $(SRC:.c=.o) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
