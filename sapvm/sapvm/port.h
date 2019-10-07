#ifndef SAPVM_PORT_H
#define SAPVM_PORT_H


#include "sapvm.h"


// ��� ������ ���������� ������������

// ������ ������ ��������
void sapvm_setup_cache(sap_ushort magic, sap_ushort size);
sap_byte sapvm_read_byte(sap_int at);
sap_short sapvm_read_short(sap_int at);
sap_int sapvm_read_int(sap_int at);
void sapvm_read_bytes(sap_int at, sap_byte *buf, sap_int size);

// ������� �������� �������, ������������ �������������
sap_byte sapvm_call_native_User(sap_vm *vm, sap_int Native_Hash, sap_thread *t, sap_frame *f, sap_object *obj, sap_byte params_len, sap_byte *params);

// ������� �� ������� ������
void sapvm_print(sap_byte *buf, sap_int size);

// �������� ������ (������ ���� ��������� ������ !)
void* sapvm_alloc(sap_int size, sap_byte what);

// ����������� ������
void sapvm_free(void *ptr);

// �������� �������� �������� ������� (���������������)
sap_uint sapvm_get_time_counter(void);

// �������� � ���������� ������� ����� (unixtime)
sap_uint sapvm_get_current_time(void);
void sapvm_set_current_time(sap_uint ut);

// ������� ms �����������
void sapvm_sleep(sap_int ms);

// �������� �������� (������ ������� ������, ���������� � ������� sapvm_alloc)
const char* sapvm_get_property(const char *name);
// ���������� �������� (�� ������ ���������� ������ �� ��� � ��������, �.�. ��� ������� ����� ������)
void sapvm_set_property(const char *name, const char *value);

// �������� ��������� �����
sap_int sapvm_rand(void);

// ����� �� ���������
void sapvm_exit(sap_int code);

// ��������� ����� 8bit � unicode
sap_ushort sapvm_8bit_to_unicode(sap_byte   b);
sap_byte   sapvm_unicode_to_8bit(sap_ushort u);

// ��������� �������� �������
sap_byte sapvm_lower_case(sap_byte b);
sap_byte sapvm_upper_case(sap_byte b);


#define SAP_HEAP_VM		0
#define SAP_HEAP_THREAD		1
#define SAP_HEAP_FRAME		2
#define SAP_HEAP_STACK		3
#define SAP_HEAP_OBJECT		4
#define SAP_HEAP_ARRAY		5


extern unsigned char sapexe[];

#define RB(at)                  ((sap_byte)sapexe[at])
#define RS(at)                  (*((sap_short*)(sapexe+(at))))
#define RI(at)                  (*((sap_int*)(sapexe+(at))))
#define RBUF(at, buf, size)     memcpy(buf, sapexe+(at), size)

/*#define RB(at)			sapvm_read_byte(at)
#define RS(at)			sapvm_read_short(at)
#define RI(at)			sapvm_read_int(at)
#define RBUF(at, buf, size)	sapvm_read_bytes(at, buf, size)*/

#define ALLOC(size, what)	sapvm_alloc(size, what)
#define FREE(ptr)		sapvm_free(ptr)

#define TIME_COUNTER()		sapvm_get_time_counter()
#define SLEEP(ms)		sapvm_sleep(ms)


#endif
