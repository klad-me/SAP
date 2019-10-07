#include "vm.h"

#include "sapvm.h"
#include "port.h"
#include "data.h"
#include "opcodes.h"
#include "gc.h"
#include "sap_native.h"
#include "sapvm_opts.h"

#include <stdio.h>
#include <string.h>
#include <math.h>



//#define DO_STACK_CHECK


// ��������� ref_count �� �������, � ���� ���� - ������� ���
#define DEC_REF(obj) \
    do	\
    {	\
	sap_object *oo=(obj);	\
	if (oo)	\
	{		\
	    oo->ref_count--;	\
	    if (oo->ref_count<=0) sapvm_gc(vm, oo);	\
	}	\
    } while(0)

void sapvm_dec_ref(sap_vm *vm, sap_object *obj)
{
    if (!obj) return;

    //printf("\t!!!DEC_REF obj->ID=0x%04x ref_count=%d\r\n",(sap_ushort)obj->ID, obj->ref_count);

    obj->ref_count--;
    if (obj->ref_count<=0)
    {
        // ����� ����� ������
        sapvm_gc(vm, obj);
    }
}


// ��������� ref_count �� �������
#define INC_REF(obj) \
    do	\
    {	\
	sap_object *oo=(obj);	\
	if (oo) oo->ref_count++;	\
    } while(0)

void sapvm_inc_ref(sap_vm *vm, sap_object *obj)
{
    if (!obj) return;
    //printf("\t!!!INC_REF obj->ID=0x%04x ref_count=%d\r\n",(sap_ushort)obj->ID, obj->ref_count);
    obj->ref_count++;
}



// ��������� ����
#if defined(DO_STACK_CHECK)
#define CHECK_PUSH(value)				\
    do							\
    {							\
	if (f->SP + (value) > f->stack_size)		\
	{						\
	    vm->fault_cause=SAP_FAULT_STACK;		\
	    return SAP_FAULT;				\
	}						\
    } while (0)

#define CHECK_POP(value)				\
    do							\
    {							\
	if (f->SP - (value) < 0)			\
	{						\
	    vm->fault_cause=SAP_FAULT_STACK;		\
	    return SAP_FAULT;				\
	}						\
    } while (0)
#else
#define CHECK_PUSH(value)
#define CHECK_POP(value)
#endif


// ��������� ������ �� null-pointer
#define CHECK_REF(obj)					\
    do							\
    {							\
	if (!(obj))					\
	{						\
	    vm->fault_cause=SAP_FAULT_NULL_REF;		\
	    return SAP_FAULT;				\
	}						\
    } while (0)


// ��������� ������ �������
#define CHECK_ARRAY(arr, idx)				\
    do							\
    {							\
	if ( ((idx)<0) || ((idx)>=((arr)->length)) )	\
	{						\
	    vm->fault_cause=SAP_FAULT_ARRAY_INDEX;	\
	    return SAP_FAULT;				\
	}						\
    } while (0)


sap_byte sapvm_execute_native_frame(sap_vm *vm, sap_thread *thr)
{
    // �������� - ���� �� ��������� �������� �����
    if (! thr->native_frame) return 0;	// ��� �������� �����
    
    // ��������� �������� �����
    if (PT_SCHEDULE(thr->native_frame->code(vm, thr))) return -1;	// ��� ��������
    
    // �������� ����� �����������
    
    // �������� ������ ������. � ���������� ������ ������������ �������� �������
    DEC_REF(thr->native_frame->obj);
    
    // �������� ���������
    sap_byte ret=thr->native_frame->result;
    sap_uint Native_Hash=thr->native_frame->Native_Hash;
    
    // ������� �������� �����
    if (thr->native_frame->data) FREE((void*)thr->native_frame->data);
    FREE((void*)thr->native_frame);
    thr->native_frame=0;
    
    if (ret>0)
    {
	// ������
	SAPVM_PRINTF("sapvm: Native thread 0x%08x fault\r\n", Native_Hash);
	vm->fault_cause=ret;
	return 1;
    }
    
    // �������� ������ ���������
    return 0;
}


sap_byte sapvm_execute_thread(sap_vm *vm, sap_thread *thr)
{
    // �������� - ���� �� ��������� �������� �����
    {
	sap_byte rv=sapvm_execute_native_frame(vm, thr);
	if (rv<0) return SAP_OK; else
	if (rv>0) return SAP_FAULT;
    }
    
    // ���� �������� ����� � ����������� ������
    sap_frame *f=thr->frame;
    
    // ����������� ��� ����� ������� ����������
    sapvm_setup_cache(f->Method_ID, f->Code_Size);
    
    // ��������� ��������� ������ �� ���
    sap_int step=STEP_N_COMMANDS;
    while (step--)
    {
	// �������� �����
	sap_ubyte op=(sap_ubyte)RB(f->Code_AT + (f->PC++));
	
#ifdef SAPVM_STATS
	// ������� ����������
	vm->bytecodes_executed_total++;
	vm->bytecodes_executed[op]++;
#endif
	
	// ���� �����
	/*{
	    int n=0;
	    
	    printf("\tStack: ");
	    while (n < f->SP)
	    {
		int addr=R_INT(f->stack + n);
		
		printf("<%d> ",addr);
		n+=4;
	    }
	    printf("\r\n");
	}*/
	
	// ���� ��������� � �������
	/*{
	    SAPVM_DEBUG("\tPC=%d SP=%d stack_size=%d op=0x%02x Code_AT=%d\t%s\r\n",
		f->PC-1,
		f->SP, f->stack_size,
		op, f->Code_AT,
		op_name[op]);
	    SLEEP(10);
	    //fflush(stdout);
	}*/
	
	// ���� ��������� � ����
	/*{
	    sapvm_show_call_stack(vm, f);
	    SLEEP(50);
	    //fflush(stdout);
	}*/
	
	// ������������ �����
	switch (op)
	{
	    // =================================================================
	    // ������ �������
	    // =================================================================
	    case OP_NOP:
		// ������ �� ������
		break;
	    
	    
	    // =================================================================
	    // �������� ��� ������
	    // =================================================================
	    case OP_B_PUSH:
		// �������� byte � ����
		CHECK_PUSH(1);
		W_BYTE(f->stack + f->SP, RB(f->Code_AT + f->PC));
		f->SP++;
		f->PC++;
		break;
	    
	    case OP_S_PUSH:
		// �������� short � ����
		CHECK_PUSH(2);
		W_SHORT(f->stack + f->SP, RS(f->Code_AT + f->PC));
		f->SP+=2;
		f->PC+=2;
		break;
	    
	    case OP_IF_PUSH:
		// �������� int/float � ����
		CHECK_PUSH(4);
		W_INT(f->stack + f->SP, RI(f->Code_AT + f->PC));
		f->SP+=4;
		f->PC+=4;
		break;
	    
	    case OP_I_PUSH_0:
		// �������� int 0 � ����
		CHECK_PUSH(4);
		W_INT(f->stack + f->SP, 0);
		f->SP+=4;
		break;
	    
	    
	    case OP_B_POP:
		// �������� byte �� �����
		CHECK_POP(1);
		f->SP--;
		break;
	    
	    case OP_S_POP:
		// �������� short �� �����
		CHECK_POP(2);
		f->SP-=2;
		break;
	    
	    case OP_IF_POP:
		// �������� int/float �� �����
		CHECK_POP(4);
		f->SP-=4;
		break;
	    
	    case OP_R_POP:
		// �������� ������ �� �����
		{
		    CHECK_POP(4);
		    f->SP-=4;
		    sap_object *ref=R_REF(f->stack + f->SP);
		    DEC_REF(ref);
		}
		break;
	    
	    
	    case OP_B_DUP:
		// ������� �������� byte � �����
		CHECK_POP(1);
		CHECK_PUSH(1);
		W_BYTE(f->stack + f->SP, R_BYTE(f->stack + f->SP-1));
		f->SP++;
		break;
	    
	    case OP_S_DUP:
		// ������� �������� short � �����
		CHECK_POP(2);
		CHECK_PUSH(2);
		W_SHORT(f->stack + f->SP, R_SHORT(f->stack + f->SP-2));
		f->SP+=2;
		break;
	    
	    case OP_IF_DUP:
		// ������� �������� int/float � �����
		CHECK_POP(4);
		CHECK_PUSH(4);
		W_INT(f->stack + f->SP, R_INT(f->stack + f->SP-4));
		f->SP+=4;
		break;
	    
	    case OP_R_DUP:
		// ������� �������� ������ � �����
		{
		    CHECK_POP(4);
		    CHECK_PUSH(4);
		    sap_object *obj=R_REF(f->stack + f->SP-4);
		    W_REF(f->stack + f->SP, obj);
		    f->SP+=4;
		    INC_REF(obj);
		}
		break;
	    
	    
	    case OP_SWAP:
		// �������� � ����� ��� int-� �������
		{
		    CHECK_POP(8);
		    sap_int tmp=R_INT(f->stack + f->SP-8);
		    W_INT(f->stack + f->SP-8, R_INT(f->stack + f->SP-4));
		    W_INT(f->stack + f->SP-4, tmp);
		}
		break;
	    
	    case OP_SWAP2:
		// �������� � ����� int-� ������ �������: <1> <2> <3> -> <2> <1> <3>
		{
		    CHECK_POP(12);
		    sap_int tmp=R_INT(f->stack + f->SP-12);
		    W_INT(f->stack + f->SP-12, R_INT(f->stack + f->SP-8));
		    W_INT(f->stack + f->SP-8, tmp);
		}
		break;
	    
	    
	    // =================================================================
	    // �������������� ������� �����
	    // =================================================================
	    case OP_B2BOOL:
		// ������������� byte � bool
		CHECK_POP(1);
		W_BYTE(f->stack + f->SP-1, R_BYTE(f->stack + f->SP-1)?1:0);
		break;
	    
	    
	    case OP_B2S:
		// ������������� byte � short
		{
		    CHECK_POP(1);
		    f->SP--;
		    sap_byte tmp=R_BYTE(f->stack + f->SP);
		    CHECK_PUSH(2);
		    W_SHORT(f->stack + f->SP, (sap_short)tmp);
		    f->SP+=2;
		}
		break;
	    
	    case OP_B2I:
		// ������������� byte � int
		{
		    CHECK_POP(1);
		    f->SP--;
		    sap_byte tmp=R_BYTE(f->stack + f->SP);
		    CHECK_PUSH(4);
		    W_INT(f->stack + f->SP, (sap_int)tmp);
		    f->SP+=4;
		}
		break;
	    
	    case OP_B2F:
		// ������������� byte � float
		{
		    CHECK_POP(1);
		    f->SP--;
		    sap_byte tmp=R_BYTE(f->stack + f->SP);
		    CHECK_PUSH(4);
		    W_FLOAT(f->stack + f->SP, (sap_float)tmp);
		    f->SP+=4;
		}
		break;
	    
	    
	    case OP_S2B:
		// ������������� short � byte
		{
		    CHECK_POP(2);
		    f->SP-=2;
		    sap_short tmp=R_SHORT(f->stack + f->SP);
		    //CHECK_PUSH(1);
		    W_BYTE(f->stack + f->SP, (sap_byte)tmp);
		    f->SP++;
		}
		break;
	    
	    case OP_S2I:
		// ������������� short � int
		{
		    CHECK_POP(2);
		    f->SP-=2;
		    sap_short tmp=R_SHORT(f->stack + f->SP);
		    CHECK_PUSH(4);
		    W_INT(f->stack + f->SP, (sap_int)tmp);
		    f->SP+=4;
		}
		break;
	    
	    case OP_S2F:
		// ������������� short � float
		{
		    CHECK_POP(2);
		    f->SP-=2;
		    sap_short tmp=R_SHORT(f->stack + f->SP);
		    CHECK_PUSH(4);
		    W_FLOAT(f->stack + f->SP, (sap_float)tmp);
		    f->SP+=4;
		}
		break;
	    
	    
	    case OP_I2B:
		// ������������� int � byte
		{
		    CHECK_POP(4);
		    f->SP-=4;
		    sap_int tmp=R_INT(f->stack + f->SP);
		    //CHECK_PUSH(1);
		    W_BYTE(f->stack + f->SP, (sap_byte)tmp);
		    f->SP++;
		}
		break;
	    
	    case OP_I2S:
		// ������������� int � short
		{
		    CHECK_POP(4);
		    f->SP-=4;
		    sap_int tmp=R_INT(f->stack + f->SP);
		    //CHECK_PUSH(2);
		    W_SHORT(f->stack + f->SP, (sap_short)tmp);
		    f->SP+=2;
		}
		break;
	    
	    case OP_I2F:
		// ������������� int � float
		{
		    CHECK_POP(4);
		    f->SP-=4;
		    sap_int tmp=R_INT(f->stack + f->SP);
		    //CHECK_PUSH(4);
		    W_FLOAT(f->stack + f->SP, (sap_float)tmp);
		    f->SP+=4;
		}
		break;
	    
	    
	    case OP_F2B:
		// ������������� float � byte
		{
		    CHECK_POP(4);
		    f->SP-=4;
		    sap_float tmp=R_FLOAT(f->stack + f->SP);
		    //CHECK_PUSH(1);
		    W_BYTE(f->stack + f->SP, (sap_byte)tmp);
		    f->SP++;
		}
		break;
	    
	    case OP_F2S:
		// ������������� float � short
		{
		    CHECK_POP(4);
		    f->SP-=4;
		    sap_float tmp=R_FLOAT(f->stack + f->SP);
		    //CHECK_PUSH(2);
		    W_SHORT(f->stack + f->SP, (sap_short)tmp);
		    f->SP+=2;
		}
		break;
	    
	    case OP_F2I:
		// ������������� float � int
		{
		    CHECK_POP(4);
		    f->SP-=4;
		    sap_float tmp=R_FLOAT(f->stack + f->SP);
		    //CHECK_PUSH(4);
		    W_INT(f->stack + f->SP, (sap_int)tmp);
		    f->SP+=4;
		}
		break;
	    
	    
	    // =================================================================
	    // ������ � ���������� �����������
	    // =================================================================
	    case OP_B_LOAD:
		// ��������� byte
		CHECK_PUSH(1);
		W_BYTE(f->stack + f->SP, R_BYTE(f->stack + RS(f->Code_AT + f->PC)));
		f->PC+=2;
		f->SP++;
		break;
	    
	    case OP_S_LOAD:
		// ��������� short
		CHECK_PUSH(2);
		W_SHORT(f->stack + f->SP, R_SHORT(f->stack + RS(f->Code_AT + f->PC)));
		f->PC+=2;
		f->SP+=2;
		break;
	    
	    case OP_IF_LOAD:
		// ��������� int/float
		CHECK_PUSH(4);
		W_INT(f->stack + f->SP, R_INT(f->stack + RS(f->Code_AT + f->PC)));
		f->PC+=2;
		f->SP+=4;
		break;
	    
	    case OP_R_LOAD:
		// ��������� ������
		{
		    CHECK_PUSH(4);
		    sap_object *obj=R_REF(f->stack + RS(f->Code_AT + f->PC));
		    W_REF(f->stack + f->SP, obj);
		    f->PC+=2;
		    f->SP+=4;
		    INC_REF(obj);
		}
		break;
	    
	    
	    case OP_B_STORE:
		// �������� byte
		CHECK_POP(1);
		W_BYTE(f->stack + RS(f->Code_AT + f->PC), R_BYTE(f->stack + f->SP-1));
		f->PC+=2;
		break;
	    
	    case OP_S_STORE:
		// �������� short
		CHECK_POP(2);
		W_SHORT(f->stack + RS(f->Code_AT + f->PC), R_SHORT(f->stack + f->SP-2));
		f->PC+=2;
		break;
	    
	    case OP_IF_STORE:
		// �������� int/float
		CHECK_POP(4);
		W_INT(f->stack + RS(f->Code_AT + f->PC), R_INT(f->stack + f->SP-4));
		f->PC+=2;
		break;
	    
	    case OP_R_STORE:
		// �������� ������
		{
		    CHECK_POP(4);
		    sap_short addr=RS(f->Code_AT + f->PC);
		    f->PC+=2;
		    sap_object *prev=R_REF(f->stack + addr);
		    sap_object *value=R_REF(f->stack + f->SP-4);
		    W_REF(f->stack + addr, value);
		    INC_REF(value);
		    DEC_REF(prev);
		}
		break;
	    
	    
	    // =================================================================
	    // ������ �� ������������ ������
	    // =================================================================
	    case OP_B_LOAD_STATIC:
		// ��������� byte
		CHECK_PUSH(1);
		W_BYTE(f->stack + f->SP, R_BYTE(vm->static_fields + RS(f->Code_AT + f->PC)));
		f->PC+=2;
		f->SP++;
		break;
	    
	    case OP_S_LOAD_STATIC:
		// ��������� short
		CHECK_PUSH(2);
		W_SHORT(f->stack + f->SP, R_SHORT(vm->static_fields + RS(f->Code_AT + f->PC)));
		f->PC+=2;
		f->SP+=2;
		break;
	    
	    case OP_IF_LOAD_STATIC:
		// ��������� int/float
		CHECK_PUSH(4);
		W_INT(f->stack + f->SP, R_INT(vm->static_fields + RS(f->Code_AT + f->PC)));
		f->PC+=2;
		f->SP+=4;
		break;
	    
	    case OP_R_LOAD_STATIC:
		// ��������� ������
		{
		    CHECK_PUSH(4);
		    sap_object *obj=R_REF(vm->static_fields + RS(f->Code_AT + f->PC));
		    W_REF(f->stack + f->SP, obj);
		    f->PC+=2;
		    f->SP+=4;
		    INC_REF(obj);
		}
		break;
	    
	    
	    case OP_B_STORE_STATIC:
		// ��������� byte
		CHECK_POP(1);
		W_BYTE(vm->static_fields + RS(f->Code_AT + f->PC), R_BYTE(f->stack + f->SP-1));
		f->PC+=2;
		break;
	    
	    case OP_S_STORE_STATIC:
		// ��������� short
		CHECK_POP(2);
		W_SHORT(vm->static_fields + RS(f->Code_AT + f->PC), R_SHORT(f->stack + f->SP-2));
		f->PC+=2;
		break;
	    
	    case OP_IF_STORE_STATIC:
		// ��������� int/float
		CHECK_POP(4);
		W_INT(vm->static_fields + RS(f->Code_AT + f->PC), R_INT(f->stack + f->SP-4));
		f->PC+=2;
		break;
	    
	    case OP_R_STORE_STATIC:
		// ��������� ������
		{
		    CHECK_POP(4);
		    sap_short addr=RS(f->Code_AT + f->PC);
		    f->PC+=2;
		    sap_object *prev=R_REF(vm->static_fields + addr);
		    sap_object *value=R_REF(f->stack + f->SP-4);
		    W_REF(vm->static_fields + addr, value);
		    INC_REF(value);
		    DEC_REF(prev);
		}
		break;
	    
	    
	    // =================================================================
	    // ������ � ������ ��������
	    // =================================================================
	    case OP_B_LOAD_FIELD:
		// ��������� byte
		{
		    CHECK_POP(4);
		    f->SP-=4;
		    sap_object *obj=R_REF(f->stack + f->SP);
		    CHECK_REF(obj);
		    //CHECK_PUSH(1);
		    W_BYTE(f->stack + f->SP, R_BYTE(obj->fields + RS(f->Code_AT + f->PC)));
		    f->PC+=2;
		    f->SP++;
		    DEC_REF(obj);
		}
		break;
	    
	    case OP_S_LOAD_FIELD:
		// ��������� short
		{
		    CHECK_POP(4);
		    f->SP-=4;
		    sap_object *obj=R_REF(f->stack + f->SP);
		    CHECK_REF(obj);
		    //CHECK_PUSH(2);
		    W_SHORT(f->stack + f->SP, R_SHORT(obj->fields + RS(f->Code_AT + f->PC)));
		    f->PC+=2;
		    f->SP+=2;
		    DEC_REF(obj);
		}
		break;
	    
	    case OP_IF_LOAD_FIELD:
		// ��������� int/float
		{
		    CHECK_POP(4);
		    f->SP-=4;
		    sap_object *obj=R_REF(f->stack + f->SP);
		    CHECK_REF(obj);
		    //CHECK_PUSH(4);
		    W_INT(f->stack + f->SP, R_INT(obj->fields + RS(f->Code_AT + f->PC)));
		    f->PC+=2;
		    f->SP+=4;
		    DEC_REF(obj);
		}
		break;
	    
	    case OP_R_LOAD_FIELD:
		// ��������� ������
		{
		    CHECK_POP(4);
		    f->SP-=4;
		    sap_object *obj=R_REF(f->stack + f->SP);
		    CHECK_REF(obj);
		    sap_object *value=R_REF(obj->fields + RS(f->Code_AT + f->PC));
		    //CHECK_PUSH(4);
		    W_REF(f->stack + f->SP, value);
		    f->PC+=2;
		    f->SP+=4;
		    INC_REF(value);
		    DEC_REF(obj);
		}
		break;
	    
	    
	    case OP_B_STORE_FIELD:
		// ��������� byte
		{
		    CHECK_POP(1+4);
		    f->SP--;
		    sap_byte value=R_BYTE(f->stack + f->SP);
		    f->SP-=4;
		    sap_object *obj=R_REF(f->stack + f->SP);
		    CHECK_REF(obj);
		    W_BYTE(obj->fields + RS(f->Code_AT + f->PC), value);
		    f->PC+=2;
		    //CHECK_PUSH(1);
		    W_BYTE(f->stack + f->SP, value);
		    f->SP++;
		    DEC_REF(obj);
		}
		break;
	    
	    
	    case OP_S_STORE_FIELD:
		// ��������� short
		{
		    CHECK_POP(2+4);
		    f->SP-=2;
		    sap_short value=R_SHORT(f->stack + f->SP);
		    f->SP-=4;
		    sap_object *obj=R_REF(f->stack + f->SP);
		    CHECK_REF(obj);
		    W_SHORT(obj->fields + RS(f->Code_AT + f->PC), value);
		    f->PC+=2;
		    //CHECK_PUSH(2);
		    W_SHORT(f->stack + f->SP, value);
		    f->SP+=2;
		    DEC_REF(obj);
		}
		break;
	    
	    
	    case OP_IF_STORE_FIELD:
		// ��������� int/float
		{
		    CHECK_POP(4+4);
		    f->SP-=4;
		    sap_int value=R_INT(f->stack + f->SP);
		    CHECK_POP(4);
		    f->SP-=4;
		    sap_object *obj=R_REF(f->stack + f->SP);
		    CHECK_REF(obj);
		    W_INT(obj->fields + RS(f->Code_AT + f->PC), value);
		    f->PC+=2;
		    //CHECK_PUSH(4);
		    W_INT(f->stack + f->SP, value);
		    f->SP+=4;
		    DEC_REF(obj);
		}
		break;
	    
	    
	    case OP_R_STORE_FIELD:
		// ��������� ������
		{
		    CHECK_POP(4+4);
		    f->SP-=4;
		    sap_object *value=R_REF(f->stack + f->SP);
		    f->SP-=4;
		    sap_object *obj=R_REF(f->stack + f->SP);
		    CHECK_REF(obj);
		    sap_short addr=RS(f->Code_AT + f->PC);
		    f->PC+=2;
		    sap_object *prev=R_REF(obj->fields + addr);
		    W_REF(obj->fields + addr, value);
		    //CHECK_PUSH(4);
		    W_REF(f->stack + f->SP, value);
		    f->SP+=4;
		    INC_REF(value);
		    DEC_REF(prev);
		    DEC_REF(obj);
		}
		break;
	    
	    
	    // =================================================================
	    // ������ � ���������
	    // =================================================================
	    case OP_B_LOAD_ARRAY:
		// ��������� byte
		{
		    CHECK_POP(4+4);
		    f->SP-=4;
		    sap_int idx=R_INT(f->stack + f->SP);
		    f->SP-=4;
		    sap_array *array=(sap_array*)R_REF(f->stack + f->SP);
		    CHECK_REF(array);
		    CHECK_ARRAY(array, idx);
		    //CHECK_PUSH(1);
		    W_BYTE(f->stack + f->SP, array->data.b[idx]);
		    f->SP++;
		    DEC_REF((sap_object*)array);
		}
		break;
	    
	    case OP_S_LOAD_ARRAY:
		// ��������� short
		{
		    CHECK_POP(4+4);
		    f->SP-=4;
		    sap_int idx=R_INT(f->stack + f->SP);
		    f->SP-=4;
		    sap_array *array=(sap_array*)R_REF(f->stack + f->SP);
		    CHECK_REF(array);
		    CHECK_ARRAY(array, idx);
		    //CHECK_PUSH(2);
		    W_SHORT(f->stack + f->SP, array->data.s[idx]);
		    f->SP+=2;
		    DEC_REF((sap_object*)array);
		}
		break;
	    
	    case OP_IF_LOAD_ARRAY:
		// ��������� int/float
		{
		    CHECK_POP(4+4);
		    f->SP-=4;
		    sap_int idx=R_INT(f->stack + f->SP);
		    f->SP-=4;
		    sap_array *array=(sap_array*)R_REF(f->stack + f->SP);
		    CHECK_REF(array);
		    CHECK_ARRAY(array, idx);
		    //CHECK_PUSH(4);
		    W_INT(f->stack + f->SP, array->data.i[idx]);
		    f->SP+=4;
		    DEC_REF((sap_object*)array);
		}
		break;
	    
	    case OP_R_LOAD_ARRAY:
		// ��������� ������
		{
		    CHECK_POP(4+4);
		    f->SP-=4;
		    sap_int idx=R_INT(f->stack + f->SP);
		    f->SP-=4;
		    sap_array *array=(sap_array*)R_REF(f->stack + f->SP);
		    CHECK_REF(array);
		    CHECK_ARRAY(array, idx);
		    sap_object *value=array->data.r[idx];
		    //CHECK_PUSH(4);
		    W_REF(f->stack + f->SP, value);
		    f->SP+=4;
		    INC_REF(value);
		    DEC_REF((sap_object*)array);
		}
		break;
	    
	    
	    case OP_B_STORE_ARRAY:
		// ��������� byte
		{
		    CHECK_POP(1+4+4);
		    f->SP--;
		    sap_byte value=R_BYTE(f->stack + f->SP);
		    f->SP-=4;
		    sap_int idx=R_INT(f->stack + f->SP);
		    f->SP-=4;
		    sap_array *array=(sap_array*)R_REF(f->stack + f->SP);
		    CHECK_REF(array);
		    CHECK_ARRAY(array, idx);
		    array->data.b[idx]=value;
		    //CHECK_PUSH(1);
		    W_BYTE(f->stack + f->SP, value);
		    f->SP++;
		    DEC_REF((sap_object*)array);
		}
		break;
	    
	    case OP_S_STORE_ARRAY:
		// ��������� short
		{
		    CHECK_POP(2+4+4);
		    f->SP-=2;
		    sap_short value=R_SHORT(f->stack + f->SP);
		    f->SP-=4;
		    sap_int idx=R_INT(f->stack + f->SP);
		    f->SP-=4;
		    sap_array *array=(sap_array*)R_REF(f->stack + f->SP);
		    CHECK_REF(array);
		    CHECK_ARRAY(array, idx);
		    array->data.s[idx]=value;
		    //CHECK_PUSH(2);
		    W_SHORT(f->stack + f->SP, value);
		    f->SP+=2;
		    DEC_REF((sap_object*)array);
		}
		break;
	    
	    case OP_IF_STORE_ARRAY:
		// ��������� int/float
		{
		    CHECK_POP(4+4+4);
		    f->SP-=4;
		    sap_int value=R_INT(f->stack + f->SP);
		    f->SP-=4;
		    sap_int idx=R_INT(f->stack + f->SP);
		    f->SP-=4;
		    sap_array *array=(sap_array*)R_REF(f->stack + f->SP);
		    CHECK_REF(array);
		    CHECK_ARRAY(array, idx);
		    array->data.i[idx]=value;
		    //CHECK_PUSH(4);
		    W_INT(f->stack + f->SP, value);
		    f->SP+=4;
		    DEC_REF((sap_object*)array);
		}
		break;
	    
	    case OP_R_STORE_ARRAY:
		// ��������� ������
		{
		    CHECK_POP(4+4+4);
		    f->SP-=4;
		    sap_object *value=R_REF(f->stack + f->SP);
		    f->SP-=4;
		    sap_int idx=R_INT(f->stack + f->SP);
		    f->SP-=4;
		    sap_array *array=(sap_array*)R_REF(f->stack + f->SP);
		    CHECK_REF(array);
		    CHECK_ARRAY(array, idx);
		    sap_object *prev=array->data.r[idx];
		    array->data.r[idx]=value;
		    //CHECK_PUSH(4);
		    W_REF(f->stack + f->SP, value);
		    f->SP+=4;
		    INC_REF(value);
		    DEC_REF(prev);
		    DEC_REF((sap_object*)array);
		}
		break;
	    
	    
	    case OP_ARRAY_LENGTH:
		// �������� ����� �������
		{
		    CHECK_POP(4);
		    f->SP-=4;
		    sap_array *array=(sap_array*)R_REF(f->stack + f->SP);
		    CHECK_REF(array);
		    //CHECK_PUSH(4)
		    W_INT(f->stack + f->SP, array->length);
		    f->SP+=4;
		    DEC_REF((sap_object*)array);
		}
		break;
	    
	    
	    case OP_FILL_ARRAY_RAW:
		// ��������� ������ �������
		{
		    CHECK_POP(4);
		    sap_array *array=(sap_array*)R_REF(f->stack + f->SP-4);
		    CHECK_REF(array);
		    sap_int len=RI(f->Code_AT + f->PC);
		    f->PC+=4;
		    RBUF(f->Code_AT + f->PC, (sap_byte*)array->data.b, len);
		    f->PC+=len;
		}
		break;
	    
	    
	    // =================================================================
	    // ����������
	    // =================================================================
	    case OP_B_ADD:
		// ������� byte
		CHECK_POP(2);
		f->SP--;
		W_BYTE(f->stack + f->SP-1, R_BYTE(f->stack + f->SP-1) + R_BYTE(f->stack + f->SP));
		break;
	    
	    case OP_S_ADD:
		// ������� short
		CHECK_POP(4);
		f->SP-=2;
		W_SHORT(f->stack + f->SP-2, R_SHORT(f->stack + f->SP-2) + R_SHORT(f->stack + f->SP));
		break;
	    
	    case OP_I_ADD:
		// ������� int
		CHECK_POP(8);
		f->SP-=4;
		W_INT(f->stack + f->SP-4, R_INT(f->stack + f->SP-4) + R_INT(f->stack + f->SP));
		break;
	    
	    case OP_F_ADD:
		// ������� float
		CHECK_POP(8);
		f->SP-=4;
		W_FLOAT(f->stack + f->SP-4, R_FLOAT(f->stack + f->SP-4) + R_FLOAT(f->stack + f->SP));
		break;
	    
	    
	    case OP_B_MUL:
		// �������� byte
		CHECK_POP(2);
		f->SP--;
		W_BYTE(f->stack + f->SP-1, R_BYTE(f->stack + f->SP-1) * R_BYTE(f->stack + f->SP));
		break;
	    
	    case OP_S_MUL:
		// �������� short
		CHECK_POP(4);
		f->SP-=2;
		W_SHORT(f->stack + f->SP-2, R_SHORT(f->stack + f->SP-2) * R_SHORT(f->stack + f->SP));
		break;
	    
	    case OP_I_MUL:
		// �������� int
		CHECK_POP(8);
		f->SP-=4;
		W_INT(f->stack + f->SP-4, R_INT(f->stack + f->SP-4) * R_INT(f->stack + f->SP));
		break;
	    
	    case OP_F_MUL:
		// �������� float
		CHECK_POP(8);
		f->SP-=4;
		W_FLOAT(f->stack + f->SP-4, R_FLOAT(f->stack + f->SP-4) * R_FLOAT(f->stack + f->SP));
		break;
	    
	    
	    case OP_B_DIV:
		// ��������� byte
		CHECK_POP(2);
		f->SP--;
		W_BYTE(f->stack + f->SP-1, R_BYTE(f->stack + f->SP-1) / R_BYTE(f->stack + f->SP));
		break;
	    
	    case OP_S_DIV:
		// ��������� short
		CHECK_POP(4);
		f->SP-=2;
		W_SHORT(f->stack + f->SP-2, R_SHORT(f->stack + f->SP-2) / R_SHORT(f->stack + f->SP));
		break;
	    
	    case OP_I_DIV:
		// ��������� int
		CHECK_POP(8);
		f->SP-=4;
		W_INT(f->stack + f->SP-4, R_INT(f->stack + f->SP-4) / R_INT(f->stack + f->SP));
		break;
	    
	    case OP_F_DIV:
		// ��������� float
		CHECK_POP(8);
		f->SP-=4;
		W_FLOAT(f->stack + f->SP-4, R_FLOAT(f->stack + f->SP-4) / R_FLOAT(f->stack + f->SP));
		break;
	    
	    
	    case OP_B_REM:
		// ������� �� ������� byte
		CHECK_POP(2);
		f->SP--;
		W_BYTE(f->stack + f->SP-1, R_BYTE(f->stack + f->SP-1) % R_BYTE(f->stack + f->SP));
		break;
	    
	    case OP_S_REM:
		// ������� �� ������� short
		CHECK_POP(4);
		f->SP-=2;
		W_SHORT(f->stack + f->SP-2, R_SHORT(f->stack + f->SP-2) % R_SHORT(f->stack + f->SP));
		break;
	    
	    case OP_I_REM:
		// ������� �� ������� int
		CHECK_POP(8);
		f->SP-=4;
		W_INT(f->stack + f->SP-4, R_INT(f->stack + f->SP-4) % R_INT(f->stack + f->SP));
		break;
	    
	    case OP_F_REM:
		// ������� �� ������� float
		CHECK_POP(8);
		f->SP-=4;
		W_FLOAT(f->stack + f->SP-4, fmodf(R_FLOAT(f->stack + f->SP-4), R_FLOAT(f->stack + f->SP)));
		break;
	    
	    
	    case OP_B_UMINUS:
		// �������� ���� byte
		CHECK_POP(1);
		W_BYTE(f->stack + f->SP-1, -R_BYTE(f->stack + f->SP-1));
		break;
	    
	    case OP_S_UMINUS:
		// �������� ���� short
		CHECK_POP(2);
		W_SHORT(f->stack + f->SP-2, -R_SHORT(f->stack + f->SP-2));
		break;
	    
	    case OP_I_UMINUS:
		// �������� ���� int
		CHECK_POP(4);
		W_INT(f->stack + f->SP-4, -R_INT(f->stack + f->SP-4));
		break;
	    
	    case OP_F_UMINUS:
		// �������� ���� float
		CHECK_POP(4);
		W_FLOAT(f->stack + f->SP-4, -R_FLOAT(f->stack + f->SP-4));
		break;
	    
	    
	    case OP_B_INC:
		// ��������� byte
		CHECK_POP(1);
		W_BYTE(f->stack + f->SP-1, R_BYTE(f->stack + f->SP-1) + 1);
		break;
	    
	    case OP_S_INC:
		// ��������� short
		CHECK_POP(2);
		W_SHORT(f->stack + f->SP-2, R_SHORT(f->stack + f->SP-2) + 1);
		break;
	    
	    case OP_I_INC:
		// ��������� int
		CHECK_POP(4);
		W_INT(f->stack + f->SP-4, R_INT(f->stack + f->SP-4) + 1);
		break;
	    
	    case OP_F_INC:
		// ��������� float
		CHECK_POP(4);
		W_FLOAT(f->stack + f->SP-4, R_FLOAT(f->stack + f->SP-4) + 1.0);
		break;
	    
	    
	    case OP_B_DEC:
		// ��������� byte
		CHECK_POP(1);
		W_BYTE(f->stack + f->SP-1, R_BYTE(f->stack + f->SP-1) - 1);
		break;
	    
	    case OP_S_DEC:
		// ��������� short
		CHECK_POP(2);
		W_SHORT(f->stack + f->SP-2, R_SHORT(f->stack + f->SP-2) - 1);
		break;
	    
	    case OP_I_DEC:
		// ��������� int
		CHECK_POP(4);
		W_INT(f->stack + f->SP-4, R_INT(f->stack + f->SP-4) - 1);
		break;
	    
	    case OP_F_DEC:
		// ��������� float
		CHECK_POP(4);
		W_FLOAT(f->stack + f->SP-4, R_FLOAT(f->stack + f->SP-4) - 1.0);
		break;
	    
	    
	    // =================================================================
	    // ������� ��������
	    // =================================================================
	    case OP_B_NEG:
		// ��������� "��" ��� byte
		CHECK_POP(1);
		W_BYTE(f->stack + f->SP-1, ~R_BYTE(f->stack + f->SP-1));
		break;
	    
	    case OP_S_NEG:
		// ��������� "��" ��� short
		CHECK_POP(2);
		W_SHORT(f->stack + f->SP-2, ~R_SHORT(f->stack + f->SP-2));
		break;
	    
	    case OP_I_NEG:
		// ��������� "��" ��� int
		CHECK_POP(4);
		W_INT(f->stack + f->SP-4, ~R_INT(f->stack + f->SP-4));
		break;
	    
	    
	    case OP_B_AND:
		// ��������� "�" ��� byte
		CHECK_POP(2);
		f->SP--;
		W_BYTE(f->stack + f->SP-1, R_BYTE(f->stack + f->SP-1) & R_BYTE(f->stack + f->SP));
		break;
	    
	    case OP_S_AND:
		// ��������� "�" ��� short
		CHECK_POP(4);
		f->SP-=2;
		W_SHORT(f->stack + f->SP-2, R_SHORT(f->stack + f->SP-2) & R_SHORT(f->stack + f->SP));
		break;
	    
	    case OP_I_AND:
		// ��������� "�" ��� int
		CHECK_POP(8);
		f->SP-=4;
		W_INT(f->stack + f->SP-4, R_INT(f->stack + f->SP-4) & R_INT(f->stack + f->SP));
		break;
	    
	    
	    case OP_B_OR:
		// ��������� "���" ��� byte
		CHECK_POP(2);
		f->SP--;
		W_BYTE(f->stack + f->SP-1, R_BYTE(f->stack + f->SP-1) | R_BYTE(f->stack + f->SP));
		break;
	    
	    case OP_S_OR:
		// ��������� "���" ��� short
		CHECK_POP(4);
		f->SP-=2;
		W_SHORT(f->stack + f->SP-2, R_SHORT(f->stack + f->SP-2) | R_SHORT(f->stack + f->SP));
		break;
	    
	    case OP_I_OR:
		// ��������� "���" ��� int
		CHECK_POP(8);
		f->SP-=4;
		W_INT(f->stack + f->SP-4, R_INT(f->stack + f->SP-4) | R_INT(f->stack + f->SP));
		break;
	    
	    
	    case OP_B_XOR:
		// ��������� "�������" ��� byte
		CHECK_POP(2);
		f->SP--;
		W_BYTE(f->stack + f->SP-1, R_BYTE(f->stack + f->SP-1) ^ R_BYTE(f->stack + f->SP));
		break;
	    
	    case OP_S_XOR:
		// ��������� "�������" ��� short
		CHECK_POP(4);
		f->SP-=2;
		W_SHORT(f->stack + f->SP-2, R_SHORT(f->stack + f->SP-2) ^ R_SHORT(f->stack + f->SP));
		break;
	    
	    case OP_I_XOR:
		// ��������� "�������" ��� int
		CHECK_POP(8);
		f->SP-=4;
		W_INT(f->stack + f->SP-4, R_INT(f->stack + f->SP-4) ^ R_INT(f->stack + f->SP));
		break;
	    
	    
	    case OP_B_SHL:
		// ����� ����� byte
		CHECK_POP(2);
		f->SP--;
		W_BYTE(f->stack + f->SP-1, R_BYTE(f->stack + f->SP-1) << R_BYTE(f->stack + f->SP));
		break;
	    
	    case OP_S_SHL:
		// ����� ����� short
		CHECK_POP(3);
		f->SP--;
		W_SHORT(f->stack + f->SP-2, R_SHORT(f->stack + f->SP-2) << R_BYTE(f->stack + f->SP));
		break;
	    
	    case OP_I_SHL:
		// ����� ����� int
		CHECK_POP(5);
		f->SP--;
		W_INT(f->stack + f->SP-4, R_INT(f->stack + f->SP-4) << R_BYTE(f->stack + f->SP));
		break;
	    
	    
	    case OP_B_SHR:
		// ����� ������ byte
		CHECK_POP(2);
		f->SP--;
		W_BYTE(f->stack + f->SP-1, R_BYTE(f->stack + f->SP-1) >> R_BYTE(f->stack + f->SP));
		break;
	    
	    case OP_S_SHR:
		// ����� ������ short
		CHECK_POP(3);
		f->SP--;
		W_SHORT(f->stack + f->SP-2, R_SHORT(f->stack + f->SP-2) >> R_BYTE(f->stack + f->SP));
		break;
	    
	    case OP_I_SHR:
		// ����� ������ int
		CHECK_POP(5);
		f->SP--;
		W_INT(f->stack + f->SP-4, R_INT(f->stack + f->SP-4) >> R_BYTE(f->stack + f->SP));
		break;
	    
	    
	    case OP_B_SHRU:
		// ����� ������ ubyte
		CHECK_POP(2);
		f->SP--;
		W_BYTE(f->stack + f->SP-1, (sap_byte)( ((sap_ubyte)R_BYTE(f->stack + f->SP-1)) >> R_BYTE(f->stack + f->SP) ) );
		break;
	    
	    case OP_S_SHRU:
		// ����� ������ ushort
		CHECK_POP(3);
		f->SP--;
		W_SHORT(f->stack + f->SP-2, (sap_short)( ((sap_ushort)R_SHORT(f->stack + f->SP-2)) >> R_BYTE(f->stack + f->SP) ) );
		break;
	    
	    case OP_I_SHRU:
		// ����� ������ uint
		CHECK_POP(5);
		f->SP--;
		W_INT(f->stack + f->SP-4, (sap_int)( ((sap_uint)R_INT(f->stack + f->SP-4)) >> R_BYTE(f->stack + f->SP) ) );
		break;
	    
	    
	    // =================================================================
	    // ���������� ��������
	    // =================================================================
	    case OP_B_LT:
		// ������ ? byte
		CHECK_POP(2);
		W_BYTE(f->stack + f->SP-2, (R_BYTE(f->stack + f->SP-2) < R_BYTE(f->stack + f->SP-1))?1:0);
		f->SP--;
		break;
	    
	    case OP_S_LT:
		// ������ ? short
		CHECK_POP(4);
		W_BYTE(f->stack + f->SP-4, (R_SHORT(f->stack + f->SP-4) < R_SHORT(f->stack + f->SP-2))?1:0);
		f->SP-=3;
		break;
	    
	    case OP_I_LT:
		// ������ ? int
		CHECK_POP(8);
		W_BYTE(f->stack + f->SP-8, (R_INT(f->stack + f->SP-8) < R_INT(f->stack + f->SP-4))?1:0);
		f->SP-=7;
		break;
	    
	    case OP_F_LT:
		// ������ ? float
		CHECK_POP(8);
		W_BYTE(f->stack + f->SP-8, (R_FLOAT(f->stack + f->SP-8) < R_FLOAT(f->stack + f->SP-4))?1:0);
		f->SP-=7;
		break;
	    
	    
	    case OP_B_GT:
		// ������ ? byte
		CHECK_POP(2);
		W_BYTE(f->stack + f->SP-2, (R_BYTE(f->stack + f->SP-2) > R_BYTE(f->stack + f->SP-1))?1:0);
		f->SP--;
		break;
	    
	    case OP_S_GT:
		// ������ ? short
		CHECK_POP(4);
		W_BYTE(f->stack + f->SP-4, (R_SHORT(f->stack + f->SP-4) > R_SHORT(f->stack + f->SP-2))?1:0);
		f->SP-=3;
		break;
	    
	    case OP_I_GT:
		// ������ ? int
		CHECK_POP(8);
		W_BYTE(f->stack + f->SP-8, (R_INT(f->stack + f->SP-8) > R_INT(f->stack + f->SP-4))?1:0);
		f->SP-=7;
		break;
	    
	    case OP_F_GT:
		// ������ ? float
		CHECK_POP(8);
		W_BYTE(f->stack + f->SP-8, (R_FLOAT(f->stack + f->SP-8) > R_FLOAT(f->stack + f->SP-4))?1:0);
		f->SP-=7;
		break;
	    
	    
	    case OP_B_EQ:
		// ����� ? byte
		CHECK_POP(2);
		W_BYTE(f->stack + f->SP-2, (R_BYTE(f->stack + f->SP-2) == R_BYTE(f->stack + f->SP-1))?1:0);
		f->SP--;
		break;
	    
	    case OP_S_EQ:
		// ����� ? short
		CHECK_POP(4);
		W_BYTE(f->stack + f->SP-4, (R_SHORT(f->stack + f->SP-4) == R_SHORT(f->stack + f->SP-2))?1:0);
		f->SP-=3;
		break;
	    
	    case OP_I_EQ:
	    case OP_F_EQ:
		// ����� ? int/float
		CHECK_POP(8);
		W_BYTE(f->stack + f->SP-8, (R_INT(f->stack + f->SP-8) == R_INT(f->stack + f->SP-4))?1:0);
		f->SP-=7;
		break;
	    
	    case OP_R_EQ:
		// ����� ? ������
		{
		    CHECK_POP(8);
		    sap_object *obj1=R_REF(f->stack + f->SP-8);
		    sap_object *obj2=R_REF(f->stack + f->SP-4);
		    W_BYTE(f->stack + f->SP-8, (obj1 == obj2)?1:0);
		    f->SP-=7;
		    DEC_REF(obj2);
		    DEC_REF(obj1);
		}
		break;
	    
	    
	    case OP_LOGIC_NOT:
		// ���������� "��" ��� byte
		CHECK_POP(1);
		W_BYTE(f->stack + f->SP-1, R_BYTE(f->stack + f->SP-1)?0:1);
		break;
	    
	    
	    // =================================================================
	    // �������� ��������
	    // =================================================================
	    case OP_JZ:
		// �������, ���� 0
		{
		    sap_short addr=RS(f->Code_AT + f->PC);
		    f->PC+=2;
		    
		    CHECK_POP(1);
		    f->SP--;
		    if (R_BYTE(f->stack + f->SP)==0) f->PC=addr;
		}
		break;
	    
	    case OP_JNZ:
		// �������, ���� �� 0
		{
		    sap_short addr=RS(f->Code_AT + f->PC);
		    f->PC+=2;
		    
		    CHECK_POP(1);
		    f->SP--;
		    if (R_BYTE(f->stack + f->SP)!=0) f->PC=addr;
		}
		break;
	    
	    case OP_JNULL:
		// �������, ���� � ����� null. ���� �� �������
		{
		    sap_short addr=RS(f->Code_AT + f->PC);
		    f->PC+=2;
		    
		    CHECK_POP(4);
		    if (R_INT(f->stack + f->SP-4)==0) f->PC=addr;
		}
		break;
	    
	    case OP_GOTO:
		// ����������� �������
		f->PC=RS(f->Code_AT + f->PC);
		break;
	    
	    case OP_CASE:
		// ������� ��� switch-case
		{
		    sap_short addr=RS(f->Code_AT + f->PC);
		    f->PC+=2;
		    
		    CHECK_POP(4);
		    f->SP-=4;
		    if (R_INT(f->stack + f->SP-4) == R_INT(f->stack + f->SP))
		    {
			// ������ �������
			f->PC=addr;
			f->SP-=4;	// �������� ��� ��������� ��������
		    }
		}
		break;
	    
	    case OP_RETURN:
		// ��������� �� ������
		{
		    // �������� ��������� ������
		    CHECK_POP(4);
		    f->SP-=4;
		    sap_int ret_value=R_INT(f->stack + f->SP);
		    
		    // �������� this
		    DEC_REF(R_REF(f->stack + 0));
		    
		    // �������� ���������� �����
		    sap_frame *prev=f->prev;
		    
		    // ������� ������� �����
		    FREE((void*)f->stack);
		    FREE((void*)f);
		    
		    // �������� �������� �������� ����������� ������
		    f=prev;
		    thr->frame=f;
		    if (f)
		    {
			// ���� ���������� �����
			CHECK_PUSH(4);
			W_INT(f->stack + f->SP, ret_value);
			f->SP+=4;
		    } else
		    {
			// ��� ����������� ������
			return SAP_THREAD_EXIT;
		    }
		    
            	    // ����������� ���
		    sapvm_setup_cache(f->Method_ID, f->Code_Size);
		}
		break;
	    
	    
	    // =================================================================
	    // �������� ������ �������
	    // =================================================================
	    case OP_CALL_VIRTUAL:
	    case OP_CALL_INTERFACE:
	    case OP_CALL_METHOD:
		// ��������� ������ �������
		{
		    // �������� ������ ����������
		    sap_byte params_len=RB(f->Code_AT + f->PC++);
		    
		    // ������� ����� ������
		    sap_short method_id=RS(f->Code_AT + f->PC);
		    f->PC+=2;
		    //printf("\tparams_len=%d method_id=%d\r\n",params_len,method_id);
		    
		    // �������� ������ �� ���������
		    CHECK_POP(params_len);
		    f->SP-=params_len;
		    sap_byte *params=(f->stack + f->SP);
		    
		    // �������� ������
		    CHECK_POP(4);
		    f->SP-=4;
		    sap_object *obj=R_REF(f->stack + f->SP);
		    
		    // ������ ��������� ���������, ���� ��������� ������ � ��������� ������
		    sap_int Class_Offs=0;
		    if ( (op == OP_CALL_VIRTUAL) ||
			 (op == OP_CALL_INTERFACE) )
		    {
			// ��������� ������
			CHECK_REF(obj);
			
			// �������� ����� ������
			Class_Offs=sizeof(sap_class_storage)*obj->ID;
			
			// ���� ��� ����� ������ ���������� - �� ���� � ������� ����� ������������ ������
			if (op == OP_CALL_INTERFACE)
			{
			    // ��� ����� ������ ����������. ���� � ������� ��������������� ����� ������������ ������
#ifndef SAPVM_CACHE
			    sap_int InterfaceBindTab_Offs=RS(vm->Classes_AT + Class_Offs + 6);
#else
			    sap_int InterfaceBindTab_Offs=R_SHORT(vm->Classes + Class_Offs + 6);
#endif
			    
			    // ���� ������������
			    sap_short if_id, v_id;
			    do
			    {
#ifndef SAPVM_CACHE
				if_id=RS(vm->InterfaceBindTab_AT + InterfaceBindTab_Offs + 0);
				v_id=RS(vm->InterfaceBindTab_AT + InterfaceBindTab_Offs + 2);
#else
				if_id=R_SHORT(vm->InterfaceBindTab + InterfaceBindTab_Offs + 0);
				v_id=R_SHORT(vm->InterfaceBindTab + InterfaceBindTab_Offs + 2);
#endif
				InterfaceBindTab_Offs+=4;
				if (if_id==method_id) break;
			    } while (if_id!=-1);
			    
			    if (v_id==-1)
			    {
				// �� ������ ����� ����������
				vm->fault_cause=SAP_FAULT_IFACE_METHOD;
				return SAP_FAULT;
			    }
			    
			    // ����������� ����� �����. �������� ������
			    method_id=v_id;
			}
			
			// �������� ����� ������ (�� ������ ������������ ������)
#ifndef SAPVM_CACHE
			sap_int VirtualTab_Offs=RS(vm->Classes_AT + Class_Offs + 4);
			method_id=RS(vm->VirtualTab_AT + VirtualTab_Offs + method_id*2);
#else
			sap_int VirtualTab_Offs=R_SHORT(vm->Classes + Class_Offs + 4);
			method_id=R_SHORT(vm->VirtualTab + VirtualTab_Offs + method_id*2);
#endif
		    }
		    
		    
		    // �������� ����� ������
		    //printf("\tmethod_id=%d params_len=%d\r\n",method_id,params_len);
		    sap_int Method_Offs=sizeof(sap_method_storage)*method_id;
		    
		    // �������� �����
#ifndef SAPVM_CACHE
		    sap_byte flags=RB(vm->Methods_AT + Method_Offs + 0);
#else
		    sap_byte flags=R_BYTE(vm->Methods + Method_Offs + 0);
#endif
		    
		    // ��������� �����
		    if (flags & 0x80)
		    {
			// ��� �������� �����
#ifndef SAPVM_CACHE
			sap_int Native_Hash=RI(vm->Methods_AT + Method_Offs + 2);
#else
			sap_int Native_Hash=R_INT(vm->Methods + Method_Offs + 2);
#endif
			// ��������� �����
			sap_byte ret=sapvm_call_native(vm, Native_Hash, thr, f, obj, params_len, params);
			if (thr->native_frame)
			{
			    // ��������� �����
			    sap_byte rv=sapvm_execute_native_frame(vm, thr);
			    if (rv<0) return SAP_OK; else
			    if (rv>0) return SAP_FAULT;
			    
			    // ����� ����� ��������� - ���������� ����������� ������
		        } else
			{
			    // ��� ��� ������������� �����
			    
			    // �������� ������ ������. � ���������� ������ ������������ �������� �������
			    DEC_REF(obj);
			    
			    if (ret>0)
			    {
				// ������
				SAPVM_PRINTF("sapvm: Native method 0x%08x fault\r\n", Native_Hash);
				vm->fault_cause=ret;
				return SAP_FAULT;
			    } else
			    if (ret<0)
			    {
				// ��������� ������������ �����
				return SAP_OK;
			    }
			}
		    } else
		    {
			// ��� ������� �����
#ifndef SAPVM_CACHE
			sap_int Frame_AT=vm->Code_AT+RI(vm->Methods_AT + Method_Offs + 2);
#else
			sap_int Frame_AT=vm->Code_AT+R_INT(vm->Methods + Method_Offs + 2);
#endif
			
			// ������� �����
			sap_frame *m=sapvm_create_frame(Frame_AT, method_id, f);
			if (!m)
			{
			    // ������ �������� ������
			    vm->fault_cause=SAP_FAULT_NO_MEMORY;
			    return SAP_FAULT;
			}
			
			// ���������, ����� ����� ������� ��� ���������
			if (m->stack_size < params_len+4)
			{
			    FREE((void*)m->stack);
			    FREE((void*)m);
			    vm->fault_cause=SAP_FAULT_STACK;
			    return SAP_FAULT;
			}
			
			// ������ ����� �������
			W_REF(m->stack + 0, obj);
			
			// ������ ���������
			memcpy((void*)(m->stack + 4), (void*)params, params_len);
			
			// ����������� ���
			sapvm_setup_cache(m->Method_ID, m->Code_Size);
			
			// �������� ���������
			m->prev=f;
			thr->frame=m;
			f=m;
		    }
		}
		break;
	    
	    
	    // =================================================================
	    // �������� ������ � ���������
	    // =================================================================
	    case OP_NEW_ARRAY:
		// ������� ������
		{
		    // �������� ������������ ����
		    sap_short spec=RS(f->Code_AT + f->PC);
		    f->PC+=2;
		    
		    //printf("new array spec=%04x\r\n",spec);
		    
		    // �������� ������ �������� ���� � �����������
		    sap_byte dim=(spec >> 8) & 0x0f;
		    sap_byte base_type=(spec >> 12) & 0x07;
		    sap_byte base_type_size;
		    switch (base_type)
		    {
			case 0:
			    // byte
			    base_type_size=1;
			    break;
			
			case 1:
			    // short
			    base_type_size=2;
			    break;
			
			case 2:
			case 3:
			    // int/float/ref
			    base_type_size=4;
			    break;
			
			default:
			    // ��
			    vm->fault_cause=SAP_FAULT_BAD_SPEC;
			    return SAP_FAULT;
		    }
		    
		    // �������� ������ ������� (���-�� ���������)
		    CHECK_POP(4);
		    f->SP-=4;
		    sap_int array_size=R_INT(f->stack + f->SP);
		    if (array_size < 0)
		    {
			// �������� ������ �������
			vm->fault_cause=SAP_FAULT_ARRAY_LENGTH;
			return SAP_FAULT;
		    }
		    
		    // �������� ������ ������ � ������
		    if (dim > 1) base_type_size=4;
		    sap_int size_bytes=base_type_size * array_size;
		    
		    // ������� ������
		    sap_array* array=(sap_array*)ALLOC(8 + size_bytes, SAP_HEAP_ARRAY);
		    if (array)
		    {
			array->ID=spec;
			array->ref_count=0;
			array->length=array_size;
			
			//printf("\tnew_array: array=%d ID=0x%04x\r\n",array,(sap_ushort)array->ID);
			
	    		// ��������� ������ ������
			memset((void*)array->data.b, 0, size_bytes);
		    }
		    
		    // ���������� ������
		    //CHECK_PUSH(4);
		    W_REF(f->stack + f->SP, array);
		    f->SP+=4;
		    INC_REF((sap_object*)array);
		}
		break;
	    
	    
	    case OP_NEW_INSTANCE:
		// ������� ����� ��������� ������
		{
		    // �������� ����� ������
		    sap_short Class_ID=RS(f->Code_AT + f->PC);
		    f->PC+=2;
		    
		    // �������� ����� ������
		    sap_int Class_Offs=sizeof(sap_class_storage)*Class_ID;
		    
		    // �������� ������ �����
#ifndef SAPVM_CACHE
		    sap_short fields_size=RS(vm->Classes_AT + Class_Offs + 0);
#else
		    sap_short fields_size=R_SHORT(vm->Classes + Class_Offs + 0);
#endif
		    
		    // ������� ���������
		    sap_object *obj=(sap_object*)ALLOC(4+fields_size, SAP_HEAP_OBJECT);
		    if (obj)
		    {
			// ������ ������
			obj->ID=Class_ID;
			obj->ref_count=0;
			memset((void*)obj->fields, 0, fields_size);
		    }
		    
		    // ���������� ������
		    CHECK_PUSH(4);
		    W_REF(f->stack + f->SP, obj);
		    f->SP+=4;
		    INC_REF(obj);
		}
		break;
	    
	    
	    case OP_CAST:
	    case OP_INSTANCEOF:
		// ������������� ��� / ��������� ���
		{
		    // �������� ������
		    CHECK_POP(4);
		    f->SP-=4;
		    sap_object *obj=R_REF(f->stack + f->SP);
		    CHECK_REF(obj);
		    
		    // �������� ������������ ����
		    sap_ushort spec=(sap_ushort)RS(f->Code_AT + f->PC);
		    f->PC+=2;
		    
		    sap_byte ok=sapvm_is_instanceof(vm, obj, spec);
		    
		    // ��������� �������
		    if (op == OP_CAST)
		    {
			// ��������� ���������� ����
			if (!ok)
			{
			    // ������
			    vm->fault_cause=SAP_FAULT_CAST;
			    return SAP_FAULT;
			}
			
			// ��� ���������. ������ ��������� ������ � �����
			W_REF(f->stack + f->SP, obj);
			f->SP+=4;
		    } else
		    {
			// ������ ��������� ���
			W_BYTE(f->stack + f->SP, ok);
			f->SP++;
			DEC_REF(obj);
		    }
		}
		break;
	    
	    
	    case OP_MONITOR_ENTER:
		// ������������� ������
		{
		    // �������� ������ ����������
		    CHECK_POP(4);
		    sap_object *obj=R_REF(f->stack + f->SP-4);
		    CHECK_REF(obj);
		    
		    // ��������� - ������������ �� ������
		    sap_lock *l=vm->locks;
		    while (l)
		    {
			if (l->obj == obj)
			{
			    // ������ ������������
			    
			    // �������� - ����� ������
			    if (l->thr != thr)
			    {
				// �� ����� ������ - �������� �����
				thr->unlock_obj=obj;
			    } else
			    {
				// ���������� ����� ������. ������ ��������� ������� ����������
				l->count++;
			    }
			    break;
			}
			l=l->next;
		    }
		    
		    if ( (!thr->unlock_obj) && (!l) )
		    {
			// ������ ����� �� ������������. ���� ������� ����� ����������
			l=(sap_lock*)ALLOC(sizeof(sap_lock), SAP_HEAP_VM);
			if (!l)
			{
			    // �� ���������� �������� ����������
			    vm->fault_cause=SAP_FAULT_NO_MEMORY;
			    return SAP_FAULT;
			}
			l->obj=obj;
			l->thr=thr;
			l->count=1;
			l->next=vm->locks;
			vm->locks=l;
		    }
		    
		    // ���� ��������������� - ����������� ������
		    if (thr->unlock_obj) return SAP_OK;
		}
		break;
	    
	    case OP_MONITOR_EXIT:
		// �������������� ������
		{
		    // �������� ������ ����������
		    CHECK_POP(4);
		    f->SP-=4;
		    sap_object *obj=R_REF(f->stack + f->SP);
		    CHECK_REF(obj);
		    
		    // ������� ����������
		    sap_lock *prev=0, *l=vm->locks;
		    while (l)
		    {
			if (l->obj == obj)
			{
			    // ����� ��������������� ������
			    l->count--;
			    if (l->count <= 0)
			    {
				// ���� ������� ����������
				
				// ��������� - ���� �����-�� ����� ����� ���� ������ - ��������� �� � ��������� ������ �� ����� �����, ����� ������� ����������
				sap_thread *t=vm->threads;
				while (t)
				{
				    if (t->unlock_obj == obj)
				    {
					t->unlock_obj=0;
					l->thr=t;
					l->count=1;
					DEC_REF(obj);
					return SAP_OK;	// ������� �������������� ������������ �����
				    }
				    t=t->next;
				}
				
				if (l->count <= 0)
				{
				    // ��� �� ���� ������� ����������
				    if (prev)
					prev->next=l->next; else
					vm->locks=l->next;
				    FREE((void*)l);
				}
			    }
			    break;
			}
			prev=l;
			l=l->next;
		    }
		    
		    // �������� ������
		    DEC_REF(obj);
		}
		break;
	    
	    
	    // =================================================================
	    // ����������� �����
	    // =================================================================
	    default:
		// ����������� �����
		vm->fault_cause=SAP_FAULT_BAD_OPCODE;
		return SAP_FAULT;
	}
    }
    
    // ��� ���������
    return SAP_OK;
}
