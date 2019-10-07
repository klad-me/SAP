#include "sap_native_Socket.h"

#include "sapvm/port.h"
#include "sapvm/data.h"
#include "sapvm/vm.h"


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>


//#define DEBUG(...)	printf(__VA_ARGS__)
#define DEBUG(...)	do {} while(0)


// Подключение
#define CONNECT_TIMEOUT	10000
typedef struct funcConnectData
{
    sap_object *obj;
    sap_array *addr;
    sap_int port;
    sap_int start_tm;
    int fd;
} funcConnectData;
static PT_THREAD(funcConnect(sap_vm *vm, sap_thread *thr))
{
    struct pt *pt=& thr->native_frame->pt;
    funcConnectData *S=(funcConnectData*)thr->native_frame->data;
    int result=0;
    
    PT_BEGIN(pt);
	
        S->start_tm=sapvm_get_time_counter();
	
	struct sockaddr_in addr;
	bzero(&addr,sizeof(addr));
	addr.sin_addr.s_addr=
	    (((sap_ubyte)S->addr->data.b[3]) << 24) |
	    (((sap_ubyte)S->addr->data.b[2]) << 16) |
	    (((sap_ubyte)S->addr->data.b[1]) << 8) |
	    (((sap_ubyte)S->addr->data.b[0]) << 0);
	addr.sin_family=AF_INET;
	addr.sin_port=htons(S->port);
	
	// Забываем адрес
	sapvm_dec_ref(vm, (sap_object*)S->addr);
	
	// Создаем сокет
	if ((S->fd=socket(AF_INET,SOCK_STREAM,0))==-1)
	{
	    // Ошибка
	    DEBUG("socket() error\n");
	    goto done;
	}
	
	// Неблокирующий режим
	int sfl=fcntl(S->fd, F_GETFL, 0);
	fcntl(S->fd, F_SETFL, sfl | O_NONBLOCK);
	
	// Подключаемся
	if (connect(S->fd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
	{
	    // Проверим ошибку
	    if(errno != EINPROGRESS) 
    	    {
    		// Ошибка
    		DEBUG("connect() failed\n");
        	close(S->fd);
        	goto done;
    	    }
    	    DEBUG("connect() in progress\n");
    	    
    	    // Ожидаем подключения
    	    while (sapvm_get_time_counter() - S->start_tm < CONNECT_TIMEOUT)
    	    {
    		// Проверим подключение
    		fd_set write_fds;
    		FD_ZERO(&write_fds);
    		FD_SET(S->fd, &write_fds);
		
    		struct timeval tv;
    		tv.tv_sec=0;
    		tv.tv_usec=0;
    		
    		int r=select(S->fd + 1, NULL, &write_fds, NULL, &tv);
    		if (r > 0)
    		{
    		    // Изменение
    		    if (FD_ISSET(S->fd, &write_fds))
    		    {
    			DEBUG("select() got change\n");
			socklen_t r_len = sizeof(r);
			if (getsockopt(S->fd, SOL_SOCKET, SO_ERROR, &r, &r_len) < 0)
			{
			    // Ошибка
			    DEBUG("getsockopt() returned error\n");
			    close(S->fd);
			    goto done;
			}
			
			if (r != 0)
			{
			    // Ошибка подключения
			    DEBUG("getsockopt() tells socket has error\n");
			    close(S->fd);
			    goto done;
			}
			
			// Подключились
			DEBUG("connect() ok\n");
			goto ok;
		    }
    		} else
    		if (r < 0)
    		{
    		    // Ошибка
    		    DEBUG("select() error\n");
    		    close(S->fd);
    		    goto done;
    		}
		
        	// Можно поспать немного
        	thr->sleep_count=1;
        	PT_YIELD(pt);
    	    }
    	    
    	    // Таймаут
    	    close(S->fd);
    	    goto done;
        }
        
	
	
ok:
	DEBUG("ok: fd=%d\n", S->fd);
	// Подключились
	result=1;
	
	// Сохраняем сокет
	W_INT(S->obj->fields+0, S->fd);
	
	

done:
	DEBUG("done:\n");
        // return result;
        W_INT(thr->frame->stack + thr->frame->SP, result);
        thr->frame->SP+=4;
	
        // Нет ошибки
        thr->native_frame->result=0;
	
    PT_END(pt);
}


// Вызвать нативную функцию Socket.*
sap_byte sapvm_call_native_Socket(sap_vm *vm, sap_int Native_Hash, sap_thread *thr, sap_frame *f, sap_object *obj, sap_byte params_len, sap_byte *params)
{
/*NativeHash: 0x5d6f13f0 for 'Socket.connect(B[I)B'
NativeHash: 0x17b98e77 for 'Socket.available()I'
NativeHash: 0x24ff50cd for 'Socket.readBytes(B[II)I'
NativeHash: 0x3d7505e8 for 'Socket.writeBytes(B[II)I'
NativeHash: 0x45df5356 for 'Socket.close()V'*/


    if (Native_Hash==0x5d6f13f0)
    {
	// native boolean Socket.connect(byte[] addr, int port);
	sap_array *addr=(sap_array*)R_REF(params+0);
	sap_int port=R_INT(params+4);
	
	if (!addr) return SAP_FAULT_NULL_REF;
	if (addr->ID != 0x8100) return SAP_FAULT_BAD_SPEC;
	
        // Создаем блокирующую задачу
        funcConnectData *data=(funcConnectData*)ALLOC(sizeof(funcConnectData), SAP_HEAP_VM);
        data->obj=obj;
        data->addr=addr;
        data->port=port;
        SAP_NATIVE_FRAME(funcConnect, data);
        return -1;
    } else
    if (Native_Hash==0x17b98e77)
    {
	// native int Socket.available();
	sap_int fd=R_INT(obj->fields+0);
	sap_int ret;
	
	DEBUG("available() fd=%d\n", fd);
	
	// Узнаем, сколько можно прочитать
	int rd_size=0;
	
    	fd_set read_fds;
    	FD_ZERO(&read_fds);
    	FD_SET(fd, &read_fds);
	
    	struct timeval tv;
    	tv.tv_sec=0;
    	tv.tv_usec=0;
    	
    	int r=select(fd + 1, &read_fds, NULL, NULL, &tv);
    	if (r > 0)
    	{
    	    if (FD_ISSET(fd, &read_fds))
    	    {
    		// Можно читать
    		size_t nbytes = 0;
    		if (ioctl(fd, FIONREAD, (char*)&nbytes) < 0 )
    		{
    		    // Ошибка
    		    rd_size=-1;
    		} else
    		{
    		    // Получили
    		    rd_size=(sap_int)nbytes;
    		}
		if (rd_size <= 0) rd_size=-1;	// закрыт
		DEBUG("select() can read rd_size=%d\n", rd_size);
    	    }
	} else
	if (r < 0)
	{
	    // Ошибка
	    DEBUG("select() error\n");
	    rd_size=-1;
	}
	
	// Возвращаем
	W_INT(f->stack + f->SP, rd_size);
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    if (Native_Hash==0x24ff50cd)
    {
	// native int Socket.read(byte[] buf, int start, int count);
	sap_int fd=R_INT(obj->fields+0);
	sap_array *buf=(sap_array*)R_REF(params+0);
	sap_int start=R_INT(params+4);
	sap_int count=R_INT(params+8);
	
	DEBUG("read() fd=%d  start=%d count=%d\n", fd, start, count);
	
	if (!buf) return SAP_FAULT_NULL_REF;
	if (buf->ID != 0x8100) return SAP_FAULT_BAD_SPEC;
	
	// Проверяем индексы
	if ( (start < 0) ||
	     (count < 0) ||
	     (start+count > buf->length) ) return SAP_FAULT_ARRAY_INDEX;
	
	// Читаем
	int rd_size=0;
	
    	fd_set read_fds;
    	FD_ZERO(&read_fds);
    	FD_SET(fd, &read_fds);
	
    	struct timeval tv;
    	tv.tv_sec=0;
    	tv.tv_usec=0;
    	
    	int r=select(fd + 1, &read_fds, NULL, NULL, &tv);
    	if (r > 0)
    	{
    	    if (FD_ISSET(fd, &read_fds))
    	    {
    		// Можно читать
		rd_size=read(fd, buf->data.b+start, count);
		if (rd_size <= 0) rd_size=-1;	// закрыт
		DEBUG("select() can read rd_size=%d\n", rd_size);
    	    }
	} else
	if (r < 0)
	{
	    // Ошибка
	    DEBUG("select() error\n");
	    rd_size=-1;
	}
	
	// Забываем буфер
	sapvm_dec_ref(vm, (sap_object*)buf);
	
	// return rd_size;
	W_INT(f->stack + f->SP, rd_size);
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    if (Native_Hash==0x3d7505e8)
    {
	// native int Socket.write(byte[] buf, int start, int count);
	sap_int fd=R_INT(obj->fields+0);
	sap_array *buf=(sap_array*)R_REF(params+0);
	sap_int start=R_INT(params+4);
	sap_int count=R_INT(params+8);
	
	DEBUG("write() fd=%d\n", fd);
	
	if (!buf) return SAP_FAULT_NULL_REF;
	if (buf->ID != 0x8100) return SAP_FAULT_BAD_SPEC;
	
	// Проверяем индексы
	if ( (start < 0) ||
	     (count < 0) ||
	     (start+count > buf->length) ) return SAP_FAULT_ARRAY_INDEX;
	
	// Пишем
	int wr_size=0;
	
    	fd_set write_fds;
    	FD_ZERO(&write_fds);
    	FD_SET(fd, &write_fds);
	
    	struct timeval tv;
    	tv.tv_sec=0;
    	tv.tv_usec=0;
    	
    	int r=select(fd + 1, NULL, &write_fds, NULL, &tv);
    	if (r > 0)
    	{
    	    if (FD_ISSET(fd, &write_fds))
    	    {
    	        // Можно писать
		wr_size=write(fd, buf->data.b+start, count);
		if (wr_size <= 0) wr_size=-1;	// закрыт
		DEBUG("select() can write wr_size=%d\n", wr_size);
    	    }
	} else
	if (r < 0)
	{
	    // Ошибка
	    DEBUG("select() error\n");
	    wr_size=-1;
	}
	
	// Забываем буфер
	sapvm_dec_ref(vm, (sap_object*)buf);
	
	// return wr_size;
	W_INT(f->stack + f->SP, wr_size);
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    if (Native_Hash==0x45df5356)
    {
	// native void Socket.close();
	sap_int fd=R_INT(obj->fields+0);
	
	DEBUG("close() fd=%d\n", fd);
	
	close(fd);
	
	W_INT(obj->fields+0, -1);
	
	// return 0;
	W_INT(f->stack + f->SP, 0);
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    
    // Нет обработчика
    return SAP_FAULT_NATIVE_NOT_FOUND;
}
