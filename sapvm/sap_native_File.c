#include "sap_native_File.h"

#include "sapvm/port.h"
#include "sapvm/data.h"
#include "sapvm/vm.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


// ���� ��� ������ �� ������ O_BINARY - ����� ������ ����� ����� ���������� !
#ifndef _WIN32
    #define O_BINARY	0
#endif


// ������� �������� ������� File.*
sap_byte sapvm_call_native_File(sap_vm *vm, sap_int Native_Hash, sap_thread *thr, sap_frame *f, sap_object *obj, sap_byte params_len, sap_byte *params)
{
    if (Native_Hash==0x4a76386e)
    {
	// native int File.__open(byte[] fname, byte mode)
	sap_array *fname=(sap_array*)R_REF(params+0);
	sap_byte mode=R_BYTE(params+4);
	
	if (!fname) return SAP_FAULT_NULL_REF;
	if (fname->ID != 0x8100) return SAP_FAULT_BAD_SPEC;
	
	int fd=-1;
	
	// ������� ��� �����
	char *str_fname=ALLOC(fname->length+1, SAP_HEAP_VM);
	if (str_fname)
	{
	    // �������� ��� �����
	    memcpy(str_fname, fname->data.b, fname->length);
	    str_fname[fname->length]=0;
	    
	    // ������ �������� ����� �� ������
	    char *ss=str_fname;
	    for ( ; *ss; ss++)
		if ((*ss)=='\\') (*ss)='/';
	    
	    if (mode)
	    {
		// ������
	        fd=open(str_fname, O_CREAT | O_TRUNC | O_RDWR | O_BINARY, 0644);
	    } else
	    {
		// ������
	        fd=open(str_fname, O_RDONLY | O_BINARY);
	    }
	    
	    FREE(str_fname);
	}
	
	// �������� ��� �����
	sapvm_dec_ref(vm, (sap_object*)fname);
	
	// return fd;
	W_INT(f->stack + f->SP, fd);
	f->SP+=4;
	
	// ��� ������
	return 0;
    } else
    if (Native_Hash==0x06856e4c)
    {
	// native void File.__seek(int fd, int pos);
	sap_int fd=R_INT(params+0);
	sap_int pos=R_INT(params+4);
	
	// ��������� �������
	lseek(fd, pos, SEEK_SET);
	
	W_INT(f->stack + f->SP, 0);
	f->SP+=4;
	
	// ��� ������
	return 0;
    } else
    if (Native_Hash==0x8a61d0ec)
    {
	// native int File.__available(int fd);
	sap_int fd=R_INT(params+0);
	
	// ������� ������� �������
	sap_int cur=lseek(fd, 0, SEEK_CUR);
	
	// ������� ������ �����
	sap_int end=lseek(fd, 0, SEEK_END);
	
	// ������������ � ������ �������
	lseek(fd, cur, SEEK_SET);
	
	// ���������� ������
	W_INT(f->stack + f->SP, end-cur);
	f->SP+=4;
	
	// ��� ������
	return 0;
    } else
    if (Native_Hash==0x78386bc7)
    {
	// native int File.__read(int fd, byte[] buf, int start, int count);
	sap_int fd=R_INT(params+0);
	sap_array *buf=(sap_array*)R_REF(params+4);
	sap_int start=R_INT(params+8);
	sap_int count=R_INT(params+12);
	
	if (!buf) return SAP_FAULT_NULL_REF;
	if (buf->ID != 0x8100) return SAP_FAULT_BAD_SPEC;
	
	// ��������� �������
	if ( (start < 0) ||
	     (count < 0) ||
	     (start+count > buf->length) ) return SAP_FAULT_ARRAY_INDEX;
	
	// ������
	int rd_size=read(fd, buf->data.b+start, count);
	
	// �������� �����
	sapvm_dec_ref(vm, (sap_object*)buf);
	
	// return rd_size;
	W_INT(f->stack + f->SP, rd_size);
	f->SP+=4;
	
	// ��� ������
	return 0;
    } else
    if (Native_Hash==0x02f089b5)
    {
	// native int File.__write(int fd, byte[] buf, int start, int count);
	sap_int fd=R_INT(params+0);
	sap_array *buf=(sap_array*)R_REF(params+4);
	sap_int start=R_INT(params+8);
	sap_int count=R_INT(params+12);
	
	if (!buf) return SAP_FAULT_NULL_REF;
	if (buf->ID != 0x8100) return SAP_FAULT_BAD_SPEC;
	
	// ��������� �������
	if ( (start < 0) ||
	     (count < 0) ||
	     (start+count > buf->length) ) return SAP_FAULT_ARRAY_INDEX;
	
	// �����
	int wr_size=write(fd, buf->data.b+start, count);
	
	// �������� �����
	sapvm_dec_ref(vm, (sap_object*)buf);
	
	// return wr_size;
	W_INT(f->stack + f->SP, wr_size);
	f->SP+=4;
	
	// ��� ������
	return 0;
    } else
    if (Native_Hash==0xdd06e430)
    {
	// native void File.__close(int fd);
	sap_int fd=R_INT(params+0);
	
	close(fd);
	
	// return 0;
	W_INT(f->stack + f->SP, 0);
	f->SP+=4;
	
	// ��� ������
	return 0;
    } else
    
    // ��� �����������
    return SAP_FAULT_NATIVE_NOT_FOUND;
}
