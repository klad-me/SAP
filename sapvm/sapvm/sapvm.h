#ifndef SAPVM_H
#define SAPVM_H


#include "types.h"
#include "sapvm_opts.h"
#include <pt.h>


// �������
enum
{
    SAP_OK=0,
    SAP_THREAD_EXIT,
    SAP_PROGRAM_EXIT,
    SAP_FAULT,
};


// ������
enum
{
    SAP_FAULT_NO_FAULT=0,
    SAP_FAULT_NO_MEMORY,
    SAP_FAULT_STACK,
    SAP_FAULT_NULL_REF,
    SAP_FAULT_ARRAY_INDEX,
    SAP_FAULT_ARRAY_LENGTH,
    SAP_FAULT_IFACE_METHOD,
    SAP_FAULT_NATIVE_NOT_FOUND,
    SAP_FAULT_BAD_SPEC,
    SAP_FAULT_CAST,
    SAP_FAULT_BAD_OPCODE,
};


// ������
typedef struct sap_object
{
    sap_ushort ID;
    sap_short ref_count;
    
    sap_byte fields[1];
} __attribute__ ((packed)) sap_object;


// ������
typedef struct sap_array
{
    sap_ushort ID;
    sap_short ref_count;
    
    sap_int length;
    
    union
    {
	sap_byte	b[1];
	sap_short	s[1];
	sap_int		i[1];
	sap_object*	r[1];
    } data;
} __attribute__ ((packed)) sap_array;


// �����
typedef struct sap_frame
{
    // �������� ����������� ������: ������� ������ � ��������� �����
    sap_short PC, SP;
    
    // ����
    sap_byte *stack;
    sap_short stack_size;
    
    // ������������ � ������ ����
    sap_int Code_AT;
    sap_ushort Code_Size;
    
    // ������������� ������������ ������ (����� �������� ���������� ��������� �� ������)
    sap_short Method_ID;
    
    // ���������� ����� (�� �������� ��� ������ ���� �����)
    struct sap_frame *prev;
} __attribute__ ((packed)) sap_frame;


// �������� ����� (����������� �������� �����, ��������� �� proto-threads)
// �������� ����� ����� ������ sleep_count ��� ���������
struct sap_vm;
struct sap_thread;
typedef PT_THREAD( (*sap_native_frame_f)(struct sap_vm*, struct sap_thread*) );
typedef struct sap_native_frame
{
    struct pt pt;		// ����� ProtoThread
    sap_object *obj;		// ������ ������
    sap_native_frame_f code;	// �������� ��� �����
    sap_uint Native_Hash;	// ������������� ������
    sap_byte result;		// ��������� ���������� 0=OK, ������ - SAP_FAULT_*
    void *data;			// ������ ��� ������ �����
} sap_native_frame;


// ������ ��� �������� ��������� ������
#define SAP_NATIVE_FRAME(func, param)	\
    do {	\
        thr->native_frame=(sap_native_frame*)ALLOC(sizeof(sap_native_frame), SAP_HEAP_FRAME);	\
        if (thr->native_frame)	\
        {	\
            PT_INIT(& thr->native_frame->pt);	\
            thr->native_frame->obj=obj;	\
            thr->native_frame->code=func;	\
            thr->native_frame->Native_Hash=Native_Hash;	\
            thr->native_frame->data=(void*)(param);	\
        }	\
    } while(0)



// �����
typedef struct sap_thread
{
    // ������� ����������� �����
    sap_frame *frame;
    
    // �������� �����
    sap_native_frame *native_frame;
    
    // ��� �����
    char name[8];
    
    // ������� ������� �������� ����� �����
    sap_uint sleep_count;
    
    // ������������� ������ ������� ���� �����
    sap_object* unlock_obj;
    
    // ��������� �� ��������� ����� � ������
    struct sap_thread *next;
} __attribute__ ((packed)) sap_thread;


// ����������
typedef struct sap_lock
{
    sap_object *obj;
    sap_thread *thr;
    sap_int count;
    
    struct sap_lock *next;
} __attribute__ ((packed)) sap_lock;


// ����������� ������
typedef struct sap_vm
{
    // ��������� ���������
    int argc;
    char **argv;
    
    // ���-�� �������
    sap_short n_classes;
    
    // ���-�� �������
    sap_short n_methods;
    
    // ������������ ������
    sap_int Boot_AT;
    sap_int Classes_AT;
    sap_int InstanceOfTab_AT;
    sap_int VirtualTab_AT;
    sap_int InterfaceBindTab_AT;
    sap_int RefFieldsTab_AT;
    sap_int Methods_AT;
    sap_int Code_AT;
    sap_int Debug_AT;
    sap_int LineNumbers_AT;
    sap_int DebugStringPool_AT;
    
#ifdef SAPVM_CACHE
    // ����������� � ��� ������ ��� ��������� ����������
    sap_byte *Classes;
    sap_byte *InstanceOfTab;
    sap_byte *VirtualTab;
    sap_byte *InterfaceBindTab;
    sap_byte *RefFieldsTab;
    sap_byte *Methods;
#endif
    
    // �������������� ����������� �������
    sap_ubyte clsid_Object;
    sap_ubyte clsid_String;
    sap_ubyte clsid_StringBuffer;
    sap_ubyte clsid_DateTime;
    
    // ������
    sap_byte fault_flag;	// ����
    sap_thread *fault_thread;	// �����, � ������� ���� ������
    sap_byte fault_cause;	// ������� ������
    
    // ����������� ����
    sap_byte *static_fields;
    sap_short n_static_refs;
    
    // ����� (��������� ������)
    sap_thread *threads;
    
    // ����������
    sap_lock *locks;
    
    // ��������� �������� �������� �������
    sap_uint last_time_counter;
    
#ifdef SAPVM_STATS
    // ����������
    sap_uint bytecodes_executed_total;
    sap_uint bytecodes_executed[256];
#endif
} __attribute__ ((packed)) sap_vm;
typedef sap_vm* SAP_VM;


// ������ Class
typedef struct sap_class_storage
{
    sap_short fields_size;
    sap_short InstanceOfTab_AT;
    sap_short VirtualTab_AT;
    sap_short InterfaceBindTab_AT;
    sap_short RefFieldsTab_AT;
} __attribute__ ((packed)) sap_class_storage;


// ������ Method
typedef struct sap_method_storage
{
    sap_byte  flags;
    sap_byte  reserved;
    union
    {
	sap_int Native_Hash;
	sap_int Code_AT;
    } data;
} __attribute__ ((packed)) sap_method_storage;


// ������ ClassDebug
typedef struct sap_classdebug_storage
{
    sap_short strName;
} __attribute__ ((packed)) sap_classdebug_storage;


// ������ MethodDebug
typedef struct sap_methoddebug_storage
{
    sap_short ParentClass_ID;
    sap_short strName;
    sap_int   LineNumbers_AT;
} __attribute__ ((packed)) sap_methoddebug_storage;



// ������� ����������� ������
SAP_VM sapvm_init(int argc, char **argv);

// ������� ����������� ������
void sapvm_done(SAP_VM vm);

// ������� �����
sap_frame* sapvm_create_frame(sap_int Frame_AT, sap_short Method_ID, sap_frame *prev);

// ������������� � ������ ����������
void sapvm_prepare_execution(SAP_VM vm);

// ������� ��� ���������� (���� �����)
sap_byte sapvm_execute_step(SAP_VM vm);

// �������� �����, ������� ����� ����������� ������ �����
sap_int sapvm_get_sleep_time(SAP_VM vm);

// ��������� ��������� �� ���������� ��� ������
sap_byte sapvm_execute(SAP_VM vm);

// ������� ��������� �� ������
void sapvm_show_error(SAP_VM vm);

// ������� ���� �������
void sapvm_show_call_stack(sap_vm *vm, sap_frame *f);

// ������� ������ �� ������� ���������� �����
void sapvm_show_StringPool_Item(sap_vm *vm, sap_short idx);

// �������� ������ �� ������� ���������� ����� (���������� ALLOC-��)
char* sapvm_get_StringPool_Item(sap_vm *vm, sap_short idx);

// �������� ������������� ������ �� ��� �����
sap_ubyte sapvm_find_class_id(sap_vm *vm, const char *name);

// �������� ��� ������ �� ��� �������������� (���������� ALLOC-��)
char* sapvm_find_class_name(sap_vm *vm, sap_ubyte id);

// ��������� - �������� �� ������ �������� ������, ������������� ��������������
sap_byte sapvm_is_instanceof(sap_vm *vm, sap_object *obj, sap_ushort spec);



// ������������� byte[] � char* (���������� � ������� ALLOC)
char* sapvm_byte_array_to_string(sap_array *arr);




#endif
