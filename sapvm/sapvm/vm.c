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


// Уменьшить ref_count на объекте, и если надо - удалить его
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
        // Можно убить объект
        sapvm_gc(vm, obj);
    }
}


// Увеличить ref_count на объекте
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



// Проверить стек
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


// Проверить ссылку на null-pointer
#define CHECK_REF(obj)					\
    do							\
    {							\
	if (!(obj))					\
	{						\
	    vm->fault_cause=SAP_FAULT_NULL_REF;		\
	    return SAP_FAULT;				\
	}						\
    } while (0)


// Проверить индекс массива
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
    // Проверим - надо ли запустить нативный фрейм
    if (! thr->native_frame) return 0;	// нет нативной нитки
    
    // Запускаем нативный фрейм
    if (PT_SCHEDULE(thr->native_frame->code(vm, thr))) return -1;	// еще работает
    
    // Нативная нитка закончилась
    
    // Забываем объект вызова. О параметрах должна позаботиться нативная функция
    DEC_REF(thr->native_frame->obj);
    
    // Получаем результат
    sap_byte ret=thr->native_frame->result;
    sap_uint Native_Hash=thr->native_frame->Native_Hash;
    
    // Удаляем нативный фрейм
    if (thr->native_frame->data) FREE((void*)thr->native_frame->data);
    FREE((void*)thr->native_frame);
    thr->native_frame=0;
    
    if (ret>0)
    {
	// Ошибка
	SAPVM_PRINTF("sapvm: Native thread 0x%08x fault\r\n", Native_Hash);
	vm->fault_cause=ret;
	return 1;
    }
    
    // Нативная задача закончена
    return 0;
}


sap_byte sapvm_execute_thread(sap_vm *vm, sap_thread *thr)
{
    // Проверим - надо ли запустить нативный фрейм
    {
	sap_byte rv=sapvm_execute_native_frame(vm, thr);
	if (rv<0) return SAP_OK; else
	if (rv>0) return SAP_FAULT;
    }
    
    // Надо выпонить фрейм в виртуальной машине
    sap_frame *f=thr->frame;
    
    // Настраиваем кэш перед началом выполнения
    sapvm_setup_cache(f->Method_ID, f->Code_Size);
    
    // Выполняем несколько команд за раз
    sap_int step=STEP_N_COMMANDS;
    while (step--)
    {
	// Получаем опкод
	sap_ubyte op=(sap_ubyte)RB(f->Code_AT + (f->PC++));
	
#ifdef SAPVM_STATS
	// Считаем статистику
	vm->bytecodes_executed_total++;
	vm->bytecodes_executed[op]++;
#endif
	
	// Дамп стека
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
	
	// Дамп регистров и команды
	/*{
	    SAPVM_DEBUG("\tPC=%d SP=%d stack_size=%d op=0x%02x Code_AT=%d\t%s\r\n",
		f->PC-1,
		f->SP, f->stack_size,
		op, f->Code_AT,
		op_name[op]);
	    SLEEP(10);
	    //fflush(stdout);
	}*/
	
	// Дамп положения в коде
	/*{
	    sapvm_show_call_stack(vm, f);
	    SLEEP(50);
	    //fflush(stdout);
	}*/
	
	// Обрабатываем опкод
	switch (op)
	{
	    // =================================================================
	    // Другие команды
	    // =================================================================
	    case OP_NOP:
		// Ничего не делать
		break;
	    
	    
	    // =================================================================
	    // Операции над стеком
	    // =================================================================
	    case OP_B_PUSH:
		// Положить byte в стек
		CHECK_PUSH(1);
		W_BYTE(f->stack + f->SP, RB(f->Code_AT + f->PC));
		f->SP++;
		f->PC++;
		break;
	    
	    case OP_S_PUSH:
		// Положить short в стек
		CHECK_PUSH(2);
		W_SHORT(f->stack + f->SP, RS(f->Code_AT + f->PC));
		f->SP+=2;
		f->PC+=2;
		break;
	    
	    case OP_IF_PUSH:
		// Положить int/float в стек
		CHECK_PUSH(4);
		W_INT(f->stack + f->SP, RI(f->Code_AT + f->PC));
		f->SP+=4;
		f->PC+=4;
		break;
	    
	    case OP_I_PUSH_0:
		// Положить int 0 в стек
		CHECK_PUSH(4);
		W_INT(f->stack + f->SP, 0);
		f->SP+=4;
		break;
	    
	    
	    case OP_B_POP:
		// Вытащить byte из стека
		CHECK_POP(1);
		f->SP--;
		break;
	    
	    case OP_S_POP:
		// Вытащить short из стека
		CHECK_POP(2);
		f->SP-=2;
		break;
	    
	    case OP_IF_POP:
		// Вытащить int/float из стека
		CHECK_POP(4);
		f->SP-=4;
		break;
	    
	    case OP_R_POP:
		// Вытащить ссылку из стека
		{
		    CHECK_POP(4);
		    f->SP-=4;
		    sap_object *ref=R_REF(f->stack + f->SP);
		    DEC_REF(ref);
		}
		break;
	    
	    
	    case OP_B_DUP:
		// Сделать дубликат byte в стеке
		CHECK_POP(1);
		CHECK_PUSH(1);
		W_BYTE(f->stack + f->SP, R_BYTE(f->stack + f->SP-1));
		f->SP++;
		break;
	    
	    case OP_S_DUP:
		// Сделать дубликат short в стеке
		CHECK_POP(2);
		CHECK_PUSH(2);
		W_SHORT(f->stack + f->SP, R_SHORT(f->stack + f->SP-2));
		f->SP+=2;
		break;
	    
	    case OP_IF_DUP:
		// Сделать дубликат int/float в стеке
		CHECK_POP(4);
		CHECK_PUSH(4);
		W_INT(f->stack + f->SP, R_INT(f->stack + f->SP-4));
		f->SP+=4;
		break;
	    
	    case OP_R_DUP:
		// Сделать дубликат ссылки в стеке
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
		// Поменять в стеке два int-а местами
		{
		    CHECK_POP(8);
		    sap_int tmp=R_INT(f->stack + f->SP-8);
		    W_INT(f->stack + f->SP-8, R_INT(f->stack + f->SP-4));
		    W_INT(f->stack + f->SP-4, tmp);
		}
		break;
	    
	    case OP_SWAP2:
		// Поменять в стеке int-ы хитрым образом: <1> <2> <3> -> <2> <1> <3>
		{
		    CHECK_POP(12);
		    sap_int tmp=R_INT(f->stack + f->SP-12);
		    W_INT(f->stack + f->SP-12, R_INT(f->stack + f->SP-8));
		    W_INT(f->stack + f->SP-8, tmp);
		}
		break;
	    
	    
	    // =================================================================
	    // Преобразования базовых типов
	    // =================================================================
	    case OP_B2BOOL:
		// Преобразовать byte в bool
		CHECK_POP(1);
		W_BYTE(f->stack + f->SP-1, R_BYTE(f->stack + f->SP-1)?1:0);
		break;
	    
	    
	    case OP_B2S:
		// Преобразовать byte в short
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
		// Преобразовать byte в int
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
		// Преобразовать byte в float
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
		// Преобразовать short в byte
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
		// Преобразовать short в int
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
		// Преобразовать short в float
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
		// Преобразовать int в byte
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
		// Преобразовать int в short
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
		// Преобразовать int в float
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
		// Преобразовать float в byte
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
		// Преобразовать float в short
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
		// Преобразовать float в int
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
	    // Работа с локальными переменными
	    // =================================================================
	    case OP_B_LOAD:
		// Загрузить byte
		CHECK_PUSH(1);
		W_BYTE(f->stack + f->SP, R_BYTE(f->stack + RS(f->Code_AT + f->PC)));
		f->PC+=2;
		f->SP++;
		break;
	    
	    case OP_S_LOAD:
		// Загрузить short
		CHECK_PUSH(2);
		W_SHORT(f->stack + f->SP, R_SHORT(f->stack + RS(f->Code_AT + f->PC)));
		f->PC+=2;
		f->SP+=2;
		break;
	    
	    case OP_IF_LOAD:
		// Загрузить int/float
		CHECK_PUSH(4);
		W_INT(f->stack + f->SP, R_INT(f->stack + RS(f->Code_AT + f->PC)));
		f->PC+=2;
		f->SP+=4;
		break;
	    
	    case OP_R_LOAD:
		// Загрузить ссылку
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
		// Записать byte
		CHECK_POP(1);
		W_BYTE(f->stack + RS(f->Code_AT + f->PC), R_BYTE(f->stack + f->SP-1));
		f->PC+=2;
		break;
	    
	    case OP_S_STORE:
		// Записать short
		CHECK_POP(2);
		W_SHORT(f->stack + RS(f->Code_AT + f->PC), R_SHORT(f->stack + f->SP-2));
		f->PC+=2;
		break;
	    
	    case OP_IF_STORE:
		// Записать int/float
		CHECK_POP(4);
		W_INT(f->stack + RS(f->Code_AT + f->PC), R_INT(f->stack + f->SP-4));
		f->PC+=2;
		break;
	    
	    case OP_R_STORE:
		// Записать ссылку
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
	    // Работа со статическими полями
	    // =================================================================
	    case OP_B_LOAD_STATIC:
		// Загрузить byte
		CHECK_PUSH(1);
		W_BYTE(f->stack + f->SP, R_BYTE(vm->static_fields + RS(f->Code_AT + f->PC)));
		f->PC+=2;
		f->SP++;
		break;
	    
	    case OP_S_LOAD_STATIC:
		// Загрузить short
		CHECK_PUSH(2);
		W_SHORT(f->stack + f->SP, R_SHORT(vm->static_fields + RS(f->Code_AT + f->PC)));
		f->PC+=2;
		f->SP+=2;
		break;
	    
	    case OP_IF_LOAD_STATIC:
		// Загрузить int/float
		CHECK_PUSH(4);
		W_INT(f->stack + f->SP, R_INT(vm->static_fields + RS(f->Code_AT + f->PC)));
		f->PC+=2;
		f->SP+=4;
		break;
	    
	    case OP_R_LOAD_STATIC:
		// Загрузить ссылку
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
		// Загрузить byte
		CHECK_POP(1);
		W_BYTE(vm->static_fields + RS(f->Code_AT + f->PC), R_BYTE(f->stack + f->SP-1));
		f->PC+=2;
		break;
	    
	    case OP_S_STORE_STATIC:
		// Загрузить short
		CHECK_POP(2);
		W_SHORT(vm->static_fields + RS(f->Code_AT + f->PC), R_SHORT(f->stack + f->SP-2));
		f->PC+=2;
		break;
	    
	    case OP_IF_STORE_STATIC:
		// Загрузить int/float
		CHECK_POP(4);
		W_INT(vm->static_fields + RS(f->Code_AT + f->PC), R_INT(f->stack + f->SP-4));
		f->PC+=2;
		break;
	    
	    case OP_R_STORE_STATIC:
		// Загрузить ссылку
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
	    // Работа с полями объектов
	    // =================================================================
	    case OP_B_LOAD_FIELD:
		// Загрузить byte
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
		// Загрузить short
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
		// Загрузить int/float
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
		// Загрузить ссылку
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
		// Загрузить byte
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
		// Загрузить short
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
		// Загрузить int/float
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
		// Загрузить ссылку
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
	    // Работа с массивами
	    // =================================================================
	    case OP_B_LOAD_ARRAY:
		// Загрузить byte
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
		// Загрузить short
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
		// Загрузить int/float
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
		// Загрузить ссылку
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
		// Загрузить byte
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
		// Загрузить short
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
		// Загрузить int/float
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
		// Загрузить ссылку
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
		// Получить длину массива
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
		// Заполнить массив байтами
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
	    // Математика
	    // =================================================================
	    case OP_B_ADD:
		// Сложить byte
		CHECK_POP(2);
		f->SP--;
		W_BYTE(f->stack + f->SP-1, R_BYTE(f->stack + f->SP-1) + R_BYTE(f->stack + f->SP));
		break;
	    
	    case OP_S_ADD:
		// Сложить short
		CHECK_POP(4);
		f->SP-=2;
		W_SHORT(f->stack + f->SP-2, R_SHORT(f->stack + f->SP-2) + R_SHORT(f->stack + f->SP));
		break;
	    
	    case OP_I_ADD:
		// Сложить int
		CHECK_POP(8);
		f->SP-=4;
		W_INT(f->stack + f->SP-4, R_INT(f->stack + f->SP-4) + R_INT(f->stack + f->SP));
		break;
	    
	    case OP_F_ADD:
		// Сложить float
		CHECK_POP(8);
		f->SP-=4;
		W_FLOAT(f->stack + f->SP-4, R_FLOAT(f->stack + f->SP-4) + R_FLOAT(f->stack + f->SP));
		break;
	    
	    
	    case OP_B_MUL:
		// Умножить byte
		CHECK_POP(2);
		f->SP--;
		W_BYTE(f->stack + f->SP-1, R_BYTE(f->stack + f->SP-1) * R_BYTE(f->stack + f->SP));
		break;
	    
	    case OP_S_MUL:
		// Умножить short
		CHECK_POP(4);
		f->SP-=2;
		W_SHORT(f->stack + f->SP-2, R_SHORT(f->stack + f->SP-2) * R_SHORT(f->stack + f->SP));
		break;
	    
	    case OP_I_MUL:
		// Умножить int
		CHECK_POP(8);
		f->SP-=4;
		W_INT(f->stack + f->SP-4, R_INT(f->stack + f->SP-4) * R_INT(f->stack + f->SP));
		break;
	    
	    case OP_F_MUL:
		// Умножить float
		CHECK_POP(8);
		f->SP-=4;
		W_FLOAT(f->stack + f->SP-4, R_FLOAT(f->stack + f->SP-4) * R_FLOAT(f->stack + f->SP));
		break;
	    
	    
	    case OP_B_DIV:
		// Разделить byte
		CHECK_POP(2);
		f->SP--;
		W_BYTE(f->stack + f->SP-1, R_BYTE(f->stack + f->SP-1) / R_BYTE(f->stack + f->SP));
		break;
	    
	    case OP_S_DIV:
		// Разделить short
		CHECK_POP(4);
		f->SP-=2;
		W_SHORT(f->stack + f->SP-2, R_SHORT(f->stack + f->SP-2) / R_SHORT(f->stack + f->SP));
		break;
	    
	    case OP_I_DIV:
		// Разделить int
		CHECK_POP(8);
		f->SP-=4;
		W_INT(f->stack + f->SP-4, R_INT(f->stack + f->SP-4) / R_INT(f->stack + f->SP));
		break;
	    
	    case OP_F_DIV:
		// Разделить float
		CHECK_POP(8);
		f->SP-=4;
		W_FLOAT(f->stack + f->SP-4, R_FLOAT(f->stack + f->SP-4) / R_FLOAT(f->stack + f->SP));
		break;
	    
	    
	    case OP_B_REM:
		// Остаток от деления byte
		CHECK_POP(2);
		f->SP--;
		W_BYTE(f->stack + f->SP-1, R_BYTE(f->stack + f->SP-1) % R_BYTE(f->stack + f->SP));
		break;
	    
	    case OP_S_REM:
		// Остаток от деления short
		CHECK_POP(4);
		f->SP-=2;
		W_SHORT(f->stack + f->SP-2, R_SHORT(f->stack + f->SP-2) % R_SHORT(f->stack + f->SP));
		break;
	    
	    case OP_I_REM:
		// Остаток от деления int
		CHECK_POP(8);
		f->SP-=4;
		W_INT(f->stack + f->SP-4, R_INT(f->stack + f->SP-4) % R_INT(f->stack + f->SP));
		break;
	    
	    case OP_F_REM:
		// Остаток от деления float
		CHECK_POP(8);
		f->SP-=4;
		W_FLOAT(f->stack + f->SP-4, fmodf(R_FLOAT(f->stack + f->SP-4), R_FLOAT(f->stack + f->SP)));
		break;
	    
	    
	    case OP_B_UMINUS:
		// Изменить знак byte
		CHECK_POP(1);
		W_BYTE(f->stack + f->SP-1, -R_BYTE(f->stack + f->SP-1));
		break;
	    
	    case OP_S_UMINUS:
		// Изменить знак short
		CHECK_POP(2);
		W_SHORT(f->stack + f->SP-2, -R_SHORT(f->stack + f->SP-2));
		break;
	    
	    case OP_I_UMINUS:
		// Изменить знак int
		CHECK_POP(4);
		W_INT(f->stack + f->SP-4, -R_INT(f->stack + f->SP-4));
		break;
	    
	    case OP_F_UMINUS:
		// Изменить знак float
		CHECK_POP(4);
		W_FLOAT(f->stack + f->SP-4, -R_FLOAT(f->stack + f->SP-4));
		break;
	    
	    
	    case OP_B_INC:
		// Увеличить byte
		CHECK_POP(1);
		W_BYTE(f->stack + f->SP-1, R_BYTE(f->stack + f->SP-1) + 1);
		break;
	    
	    case OP_S_INC:
		// Увеличить short
		CHECK_POP(2);
		W_SHORT(f->stack + f->SP-2, R_SHORT(f->stack + f->SP-2) + 1);
		break;
	    
	    case OP_I_INC:
		// Увеличить int
		CHECK_POP(4);
		W_INT(f->stack + f->SP-4, R_INT(f->stack + f->SP-4) + 1);
		break;
	    
	    case OP_F_INC:
		// Увеличить float
		CHECK_POP(4);
		W_FLOAT(f->stack + f->SP-4, R_FLOAT(f->stack + f->SP-4) + 1.0);
		break;
	    
	    
	    case OP_B_DEC:
		// Уменьшить byte
		CHECK_POP(1);
		W_BYTE(f->stack + f->SP-1, R_BYTE(f->stack + f->SP-1) - 1);
		break;
	    
	    case OP_S_DEC:
		// Уменьшить short
		CHECK_POP(2);
		W_SHORT(f->stack + f->SP-2, R_SHORT(f->stack + f->SP-2) - 1);
		break;
	    
	    case OP_I_DEC:
		// Уменьшить int
		CHECK_POP(4);
		W_INT(f->stack + f->SP-4, R_INT(f->stack + f->SP-4) - 1);
		break;
	    
	    case OP_F_DEC:
		// Уменьшить float
		CHECK_POP(4);
		W_FLOAT(f->stack + f->SP-4, R_FLOAT(f->stack + f->SP-4) - 1.0);
		break;
	    
	    
	    // =================================================================
	    // Битовые операции
	    // =================================================================
	    case OP_B_NEG:
		// Побитовое "НЕ" для byte
		CHECK_POP(1);
		W_BYTE(f->stack + f->SP-1, ~R_BYTE(f->stack + f->SP-1));
		break;
	    
	    case OP_S_NEG:
		// Побитовое "НЕ" для short
		CHECK_POP(2);
		W_SHORT(f->stack + f->SP-2, ~R_SHORT(f->stack + f->SP-2));
		break;
	    
	    case OP_I_NEG:
		// Побитовое "НЕ" для int
		CHECK_POP(4);
		W_INT(f->stack + f->SP-4, ~R_INT(f->stack + f->SP-4));
		break;
	    
	    
	    case OP_B_AND:
		// Побитовое "И" для byte
		CHECK_POP(2);
		f->SP--;
		W_BYTE(f->stack + f->SP-1, R_BYTE(f->stack + f->SP-1) & R_BYTE(f->stack + f->SP));
		break;
	    
	    case OP_S_AND:
		// Побитовое "И" для short
		CHECK_POP(4);
		f->SP-=2;
		W_SHORT(f->stack + f->SP-2, R_SHORT(f->stack + f->SP-2) & R_SHORT(f->stack + f->SP));
		break;
	    
	    case OP_I_AND:
		// Побитовое "И" для int
		CHECK_POP(8);
		f->SP-=4;
		W_INT(f->stack + f->SP-4, R_INT(f->stack + f->SP-4) & R_INT(f->stack + f->SP));
		break;
	    
	    
	    case OP_B_OR:
		// Побитовое "ИЛИ" для byte
		CHECK_POP(2);
		f->SP--;
		W_BYTE(f->stack + f->SP-1, R_BYTE(f->stack + f->SP-1) | R_BYTE(f->stack + f->SP));
		break;
	    
	    case OP_S_OR:
		// Побитовое "ИЛИ" для short
		CHECK_POP(4);
		f->SP-=2;
		W_SHORT(f->stack + f->SP-2, R_SHORT(f->stack + f->SP-2) | R_SHORT(f->stack + f->SP));
		break;
	    
	    case OP_I_OR:
		// Побитовое "ИЛИ" для int
		CHECK_POP(8);
		f->SP-=4;
		W_INT(f->stack + f->SP-4, R_INT(f->stack + f->SP-4) | R_INT(f->stack + f->SP));
		break;
	    
	    
	    case OP_B_XOR:
		// Побитовое "исклИЛИ" для byte
		CHECK_POP(2);
		f->SP--;
		W_BYTE(f->stack + f->SP-1, R_BYTE(f->stack + f->SP-1) ^ R_BYTE(f->stack + f->SP));
		break;
	    
	    case OP_S_XOR:
		// Побитовое "исклИЛИ" для short
		CHECK_POP(4);
		f->SP-=2;
		W_SHORT(f->stack + f->SP-2, R_SHORT(f->stack + f->SP-2) ^ R_SHORT(f->stack + f->SP));
		break;
	    
	    case OP_I_XOR:
		// Побитовое "исклИЛИ" для int
		CHECK_POP(8);
		f->SP-=4;
		W_INT(f->stack + f->SP-4, R_INT(f->stack + f->SP-4) ^ R_INT(f->stack + f->SP));
		break;
	    
	    
	    case OP_B_SHL:
		// Сдвиг влево byte
		CHECK_POP(2);
		f->SP--;
		W_BYTE(f->stack + f->SP-1, R_BYTE(f->stack + f->SP-1) << R_BYTE(f->stack + f->SP));
		break;
	    
	    case OP_S_SHL:
		// Сдвиг влево short
		CHECK_POP(3);
		f->SP--;
		W_SHORT(f->stack + f->SP-2, R_SHORT(f->stack + f->SP-2) << R_BYTE(f->stack + f->SP));
		break;
	    
	    case OP_I_SHL:
		// Сдвиг влево int
		CHECK_POP(5);
		f->SP--;
		W_INT(f->stack + f->SP-4, R_INT(f->stack + f->SP-4) << R_BYTE(f->stack + f->SP));
		break;
	    
	    
	    case OP_B_SHR:
		// Сдвиг вправо byte
		CHECK_POP(2);
		f->SP--;
		W_BYTE(f->stack + f->SP-1, R_BYTE(f->stack + f->SP-1) >> R_BYTE(f->stack + f->SP));
		break;
	    
	    case OP_S_SHR:
		// Сдвиг вправо short
		CHECK_POP(3);
		f->SP--;
		W_SHORT(f->stack + f->SP-2, R_SHORT(f->stack + f->SP-2) >> R_BYTE(f->stack + f->SP));
		break;
	    
	    case OP_I_SHR:
		// Сдвиг вправо int
		CHECK_POP(5);
		f->SP--;
		W_INT(f->stack + f->SP-4, R_INT(f->stack + f->SP-4) >> R_BYTE(f->stack + f->SP));
		break;
	    
	    
	    case OP_B_SHRU:
		// Сдвиг вправо ubyte
		CHECK_POP(2);
		f->SP--;
		W_BYTE(f->stack + f->SP-1, (sap_byte)( ((sap_ubyte)R_BYTE(f->stack + f->SP-1)) >> R_BYTE(f->stack + f->SP) ) );
		break;
	    
	    case OP_S_SHRU:
		// Сдвиг вправо ushort
		CHECK_POP(3);
		f->SP--;
		W_SHORT(f->stack + f->SP-2, (sap_short)( ((sap_ushort)R_SHORT(f->stack + f->SP-2)) >> R_BYTE(f->stack + f->SP) ) );
		break;
	    
	    case OP_I_SHRU:
		// Сдвиг вправо uint
		CHECK_POP(5);
		f->SP--;
		W_INT(f->stack + f->SP-4, (sap_int)( ((sap_uint)R_INT(f->stack + f->SP-4)) >> R_BYTE(f->stack + f->SP) ) );
		break;
	    
	    
	    // =================================================================
	    // Логические операции
	    // =================================================================
	    case OP_B_LT:
		// Меньше ? byte
		CHECK_POP(2);
		W_BYTE(f->stack + f->SP-2, (R_BYTE(f->stack + f->SP-2) < R_BYTE(f->stack + f->SP-1))?1:0);
		f->SP--;
		break;
	    
	    case OP_S_LT:
		// Меньше ? short
		CHECK_POP(4);
		W_BYTE(f->stack + f->SP-4, (R_SHORT(f->stack + f->SP-4) < R_SHORT(f->stack + f->SP-2))?1:0);
		f->SP-=3;
		break;
	    
	    case OP_I_LT:
		// Меньше ? int
		CHECK_POP(8);
		W_BYTE(f->stack + f->SP-8, (R_INT(f->stack + f->SP-8) < R_INT(f->stack + f->SP-4))?1:0);
		f->SP-=7;
		break;
	    
	    case OP_F_LT:
		// Меньше ? float
		CHECK_POP(8);
		W_BYTE(f->stack + f->SP-8, (R_FLOAT(f->stack + f->SP-8) < R_FLOAT(f->stack + f->SP-4))?1:0);
		f->SP-=7;
		break;
	    
	    
	    case OP_B_GT:
		// Больше ? byte
		CHECK_POP(2);
		W_BYTE(f->stack + f->SP-2, (R_BYTE(f->stack + f->SP-2) > R_BYTE(f->stack + f->SP-1))?1:0);
		f->SP--;
		break;
	    
	    case OP_S_GT:
		// Больше ? short
		CHECK_POP(4);
		W_BYTE(f->stack + f->SP-4, (R_SHORT(f->stack + f->SP-4) > R_SHORT(f->stack + f->SP-2))?1:0);
		f->SP-=3;
		break;
	    
	    case OP_I_GT:
		// Больше ? int
		CHECK_POP(8);
		W_BYTE(f->stack + f->SP-8, (R_INT(f->stack + f->SP-8) > R_INT(f->stack + f->SP-4))?1:0);
		f->SP-=7;
		break;
	    
	    case OP_F_GT:
		// Больше ? float
		CHECK_POP(8);
		W_BYTE(f->stack + f->SP-8, (R_FLOAT(f->stack + f->SP-8) > R_FLOAT(f->stack + f->SP-4))?1:0);
		f->SP-=7;
		break;
	    
	    
	    case OP_B_EQ:
		// Равно ? byte
		CHECK_POP(2);
		W_BYTE(f->stack + f->SP-2, (R_BYTE(f->stack + f->SP-2) == R_BYTE(f->stack + f->SP-1))?1:0);
		f->SP--;
		break;
	    
	    case OP_S_EQ:
		// Равно ? short
		CHECK_POP(4);
		W_BYTE(f->stack + f->SP-4, (R_SHORT(f->stack + f->SP-4) == R_SHORT(f->stack + f->SP-2))?1:0);
		f->SP-=3;
		break;
	    
	    case OP_I_EQ:
	    case OP_F_EQ:
		// Равно ? int/float
		CHECK_POP(8);
		W_BYTE(f->stack + f->SP-8, (R_INT(f->stack + f->SP-8) == R_INT(f->stack + f->SP-4))?1:0);
		f->SP-=7;
		break;
	    
	    case OP_R_EQ:
		// Равно ? ссылка
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
		// Логическое "НЕ" для byte
		CHECK_POP(1);
		W_BYTE(f->stack + f->SP-1, R_BYTE(f->stack + f->SP-1)?0:1);
		break;
	    
	    
	    // =================================================================
	    // Операции перехода
	    // =================================================================
	    case OP_JZ:
		// Перейти, если 0
		{
		    sap_short addr=RS(f->Code_AT + f->PC);
		    f->PC+=2;
		    
		    CHECK_POP(1);
		    f->SP--;
		    if (R_BYTE(f->stack + f->SP)==0) f->PC=addr;
		}
		break;
	    
	    case OP_JNZ:
		// Перейти, если не 0
		{
		    sap_short addr=RS(f->Code_AT + f->PC);
		    f->PC+=2;
		    
		    CHECK_POP(1);
		    f->SP--;
		    if (R_BYTE(f->stack + f->SP)!=0) f->PC=addr;
		}
		break;
	    
	    case OP_JNULL:
		// Перейти, если в стеке null. Стек не трогать
		{
		    sap_short addr=RS(f->Code_AT + f->PC);
		    f->PC+=2;
		    
		    CHECK_POP(4);
		    if (R_INT(f->stack + f->SP-4)==0) f->PC=addr;
		}
		break;
	    
	    case OP_GOTO:
		// Безусловный переход
		f->PC=RS(f->Code_AT + f->PC);
		break;
	    
	    case OP_CASE:
		// Переход для switch-case
		{
		    sap_short addr=RS(f->Code_AT + f->PC);
		    f->PC+=2;
		    
		    CHECK_POP(4);
		    f->SP-=4;
		    if (R_INT(f->stack + f->SP-4) == R_INT(f->stack + f->SP))
		    {
			// Делаем переход
			f->PC=addr;
			f->SP-=4;	// значение для сравнения забываем
		    }
		}
		break;
	    
	    case OP_RETURN:
		// Вернуться из метода
		{
		    // Получаем результат метода
		    CHECK_POP(4);
		    f->SP-=4;
		    sap_int ret_value=R_INT(f->stack + f->SP);
		    
		    // Забываем this
		    DEC_REF(R_REF(f->stack + 0));
		    
		    // Получаем вызывающий фрейм
		    sap_frame *prev=f->prev;
		    
		    // Удаляем текущий фрейм
		    FREE((void*)f->stack);
		    FREE((void*)f);
		    
		    // Передаем значение возврата вызывающему фрейму
		    f=prev;
		    thr->frame=f;
		    if (f)
		    {
			// Есть вызывающий фрейм
			CHECK_PUSH(4);
			W_INT(f->stack + f->SP, ret_value);
			f->SP+=4;
		    } else
		    {
			// Нет вызывающего фрейма
			return SAP_THREAD_EXIT;
		    }
		    
            	    // Настраиваем кэш
		    sapvm_setup_cache(f->Method_ID, f->Code_Size);
		}
		break;
	    
	    
	    // =================================================================
	    // Операции вызова методов
	    // =================================================================
	    case OP_CALL_VIRTUAL:
	    case OP_CALL_INTERFACE:
	    case OP_CALL_METHOD:
		// Различные вызовы методов
		{
		    // Получаем размер параметров
		    sap_byte params_len=RB(f->Code_AT + f->PC++);
		    
		    // Получим номер метода
		    sap_short method_id=RS(f->Code_AT + f->PC);
		    f->PC+=2;
		    //printf("\tparams_len=%d method_id=%d\r\n",params_len,method_id);
		    
		    // Получаем ссылку на параметры
		    CHECK_POP(params_len);
		    f->SP-=params_len;
		    sap_byte *params=(f->stack + f->SP);
		    
		    // Получаем объект
		    CHECK_POP(4);
		    f->SP-=4;
		    sap_object *obj=R_REF(f->stack + f->SP);
		    
		    // Делаем отдельную обработку, если требуется работа с таблицами класса
		    sap_int Class_Offs=0;
		    if ( (op == OP_CALL_VIRTUAL) ||
			 (op == OP_CALL_INTERFACE) )
		    {
			// Проверяем объект
			CHECK_REF(obj);
			
			// Получаем адрес класса
			Class_Offs=sizeof(sap_class_storage)*obj->ID;
			
			// Если это вызов метода интерфейса - то ищем в таблице номер виртуального метода
			if (op == OP_CALL_INTERFACE)
			{
			    // Это вызов метода интерфейса. Ищем в таблице соответствующий номер виртуального метода
#ifndef SAPVM_CACHE
			    sap_int InterfaceBindTab_Offs=RS(vm->Classes_AT + Class_Offs + 6);
#else
			    sap_int InterfaceBindTab_Offs=R_SHORT(vm->Classes + Class_Offs + 6);
#endif
			    
			    // Ищем соответствие
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
				// Не найден метод интерфейса
				vm->fault_cause=SAP_FAULT_IFACE_METHOD;
				return SAP_FAULT;
			    }
			    
			    // Виртуальный метод нашли. Работаем дальше
			    method_id=v_id;
			}
			
			// Получаем номер метода (из номера виртуального метода)
#ifndef SAPVM_CACHE
			sap_int VirtualTab_Offs=RS(vm->Classes_AT + Class_Offs + 4);
			method_id=RS(vm->VirtualTab_AT + VirtualTab_Offs + method_id*2);
#else
			sap_int VirtualTab_Offs=R_SHORT(vm->Classes + Class_Offs + 4);
			method_id=R_SHORT(vm->VirtualTab + VirtualTab_Offs + method_id*2);
#endif
		    }
		    
		    
		    // Получаем адрес метода
		    //printf("\tmethod_id=%d params_len=%d\r\n",method_id,params_len);
		    sap_int Method_Offs=sizeof(sap_method_storage)*method_id;
		    
		    // Получаем флаги
#ifndef SAPVM_CACHE
		    sap_byte flags=RB(vm->Methods_AT + Method_Offs + 0);
#else
		    sap_byte flags=R_BYTE(vm->Methods + Method_Offs + 0);
#endif
		    
		    // Запускаем метод
		    if (flags & 0x80)
		    {
			// Это нативный метод
#ifndef SAPVM_CACHE
			sap_int Native_Hash=RI(vm->Methods_AT + Method_Offs + 2);
#else
			sap_int Native_Hash=R_INT(vm->Methods + Method_Offs + 2);
#endif
			// Запускаем метод
			sap_byte ret=sapvm_call_native(vm, Native_Hash, thr, f, obj, params_len, params);
			if (thr->native_frame)
			{
			    // Запустили нитку
			    sap_byte rv=sapvm_execute_native_frame(vm, thr);
			    if (rv<0) return SAP_OK; else
			    if (rv>0) return SAP_FAULT;
			    
			    // Нитка сразу вернулась - продолжаем виртуальную машину
		        } else
			{
			    // Это был неблокирующий вызов
			    
			    // Забываем объект вызова. О параметрах должна позаботиться нативная функция
			    DEC_REF(obj);
			    
			    if (ret>0)
			    {
				// Ошибка
				SAPVM_PRINTF("sapvm: Native method 0x%08x fault\r\n", Native_Hash);
				vm->fault_cause=ret;
				return SAP_FAULT;
			    } else
			    if (ret<0)
			    {
				// Требуется переключение задач
				return SAP_OK;
			    }
			}
		    } else
		    {
			// Это обычный метод
#ifndef SAPVM_CACHE
			sap_int Frame_AT=vm->Code_AT+RI(vm->Methods_AT + Method_Offs + 2);
#else
			sap_int Frame_AT=vm->Code_AT+R_INT(vm->Methods + Method_Offs + 2);
#endif
			
			// Создаем фрейм
			sap_frame *m=sapvm_create_frame(Frame_AT, method_id, f);
			if (!m)
			{
			    // Ошибка создания фрейма
			    vm->fault_cause=SAP_FAULT_NO_MEMORY;
			    return SAP_FAULT;
			}
			
			// Проверяем, чтобы фрейм подошел под параметры
			if (m->stack_size < params_len+4)
			{
			    FREE((void*)m->stack);
			    FREE((void*)m);
			    vm->fault_cause=SAP_FAULT_STACK;
			    return SAP_FAULT;
			}
			
			// Кладем адрес объекта
			W_REF(m->stack + 0, obj);
			
			// Кладем параметры
			memcpy((void*)(m->stack + 4), (void*)params, params_len);
			
			// Настраиваем кэш
			sapvm_setup_cache(m->Method_ID, m->Code_Size);
			
			// Начинаем выполнять
			m->prev=f;
			thr->frame=m;
			f=m;
		    }
		}
		break;
	    
	    
	    // =================================================================
	    // Операции работы с объектами
	    // =================================================================
	    case OP_NEW_ARRAY:
		// Создать массив
		{
		    // Получаем спецификатор типа
		    sap_short spec=RS(f->Code_AT + f->PC);
		    f->PC+=2;
		    
		    //printf("new array spec=%04x\r\n",spec);
		    
		    // Получаем размер базового типа и размерность
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
			    // ХЗ
			    vm->fault_cause=SAP_FAULT_BAD_SPEC;
			    return SAP_FAULT;
		    }
		    
		    // Получаем размер массива (кол-во элементов)
		    CHECK_POP(4);
		    f->SP-=4;
		    sap_int array_size=R_INT(f->stack + f->SP);
		    if (array_size < 0)
		    {
			// Неверный размер массива
			vm->fault_cause=SAP_FAULT_ARRAY_LENGTH;
			return SAP_FAULT;
		    }
		    
		    // Получаем размер памяти в байтах
		    if (dim > 1) base_type_size=4;
		    sap_int size_bytes=base_type_size * array_size;
		    
		    // Создаем массив
		    sap_array* array=(sap_array*)ALLOC(8 + size_bytes, SAP_HEAP_ARRAY);
		    if (array)
		    {
			array->ID=spec;
			array->ref_count=0;
			array->length=array_size;
			
			//printf("\tnew_array: array=%d ID=0x%04x\r\n",array,(sap_ushort)array->ID);
			
	    		// Заполняем нулями данные
			memset((void*)array->data.b, 0, size_bytes);
		    }
		    
		    // Возвращаем ссылку
		    //CHECK_PUSH(4);
		    W_REF(f->stack + f->SP, array);
		    f->SP+=4;
		    INC_REF((sap_object*)array);
		}
		break;
	    
	    
	    case OP_NEW_INSTANCE:
		// Создать новый экземпляр класса
		{
		    // Получаем номер класса
		    sap_short Class_ID=RS(f->Code_AT + f->PC);
		    f->PC+=2;
		    
		    // Получаем адрес класса
		    sap_int Class_Offs=sizeof(sap_class_storage)*Class_ID;
		    
		    // Получаем размер полей
#ifndef SAPVM_CACHE
		    sap_short fields_size=RS(vm->Classes_AT + Class_Offs + 0);
#else
		    sap_short fields_size=R_SHORT(vm->Classes + Class_Offs + 0);
#endif
		    
		    // Создаем экземпляр
		    sap_object *obj=(sap_object*)ALLOC(4+fields_size, SAP_HEAP_OBJECT);
		    if (obj)
		    {
			// Инитим объект
			obj->ID=Class_ID;
			obj->ref_count=0;
			memset((void*)obj->fields, 0, fields_size);
		    }
		    
		    // Возвращаем ссылку
		    CHECK_PUSH(4);
		    W_REF(f->stack + f->SP, obj);
		    f->SP+=4;
		    INC_REF(obj);
		}
		break;
	    
	    
	    case OP_CAST:
	    case OP_INSTANCEOF:
		// Преобразовать тип / проверить тип
		{
		    // Получаем объект
		    CHECK_POP(4);
		    f->SP-=4;
		    sap_object *obj=R_REF(f->stack + f->SP);
		    CHECK_REF(obj);
		    
		    // Получаем спецификатор типа
		    sap_ushort spec=(sap_ushort)RS(f->Code_AT + f->PC);
		    f->PC+=2;
		    
		    sap_byte ok=sapvm_is_instanceof(vm, obj, spec);
		    
		    // Принимаем решение
		    if (op == OP_CAST)
		    {
			// Требуется приведение типа
			if (!ok)
			{
			    // Ошибка
			    vm->fault_cause=SAP_FAULT_CAST;
			    return SAP_FAULT;
			}
			
			// Все нормально. Просто оставляем ссылку в стеке
			W_REF(f->stack + f->SP, obj);
			f->SP+=4;
		    } else
		    {
			// Просто проверяли тип
			W_BYTE(f->stack + f->SP, ok);
			f->SP++;
			DEC_REF(obj);
		    }
		}
		break;
	    
	    
	    case OP_MONITOR_ENTER:
		// Заблокировать объект
		{
		    // Получаем объект блокировки
		    CHECK_POP(4);
		    sap_object *obj=R_REF(f->stack + f->SP-4);
		    CHECK_REF(obj);
		    
		    // Проверяем - заблокирован ли объект
		    sap_lock *l=vm->locks;
		    while (l)
		    {
			if (l->obj == obj)
			{
			    // Объект заблокирован
			    
			    // Проверим - какой ниткой
			    if (l->thr != thr)
			    {
				// Не нашей ниткой - придется ждать
				thr->unlock_obj=obj;
			    } else
			    {
				// Блокировка нашей ниткой. Просто добавляем счетчик блокировок
				l->count++;
			    }
			    break;
			}
			l=l->next;
		    }
		    
		    if ( (!thr->unlock_obj) && (!l) )
		    {
			// Объект никем не заблокирован. Надо создать новую блокировку
			l=(sap_lock*)ALLOC(sizeof(sap_lock), SAP_HEAP_VM);
			if (!l)
			{
			    // Не получилось выделить блокировку
			    vm->fault_cause=SAP_FAULT_NO_MEMORY;
			    return SAP_FAULT;
			}
			l->obj=obj;
			l->thr=thr;
			l->count=1;
			l->next=vm->locks;
			vm->locks=l;
		    }
		    
		    // Если заблокировались - переключаем задачи
		    if (thr->unlock_obj) return SAP_OK;
		}
		break;
	    
	    case OP_MONITOR_EXIT:
		// Разблокировать объект
		{
		    // Получаем объект блокировки
		    CHECK_POP(4);
		    f->SP-=4;
		    sap_object *obj=R_REF(f->stack + f->SP);
		    CHECK_REF(obj);
		    
		    // Снимаем блокировку
		    sap_lock *prev=0, *l=vm->locks;
		    while (l)
		    {
			if (l->obj == obj)
			{
			    // Нашли заблокированный объект
			    l->count--;
			    if (l->count <= 0)
			    {
				// Надо удалить блокировку
				
				// Проверяем - если какая-то нитка ждала этот объект - просыпаем ее и блокируем объект на новую нитку, иначе удаляем блокировку
				sap_thread *t=vm->threads;
				while (t)
				{
				    if (t->unlock_obj == obj)
				    {
					t->unlock_obj=0;
					l->thr=t;
					l->count=1;
					DEC_REF(obj);
					return SAP_OK;	// сделаем принудительное переключение ниток
				    }
				    t=t->next;
				}
				
				if (l->count <= 0)
				{
				    // Все же надо удалить блокировку
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
		    
		    // Забываем объект
		    DEC_REF(obj);
		}
		break;
	    
	    
	    // =================================================================
	    // Неизвестный опкод
	    // =================================================================
	    default:
		// Неизвестный опкод
		vm->fault_cause=SAP_FAULT_BAD_OPCODE;
		return SAP_FAULT;
	}
    }
    
    // Все нормально
    return SAP_OK;
}
