#include "sap_native.h"

#include "port.h"
#include "data.h"
#include "vm.h"
#include "sapvm_opts.h"

#include <stdio.h>
#include <string.h>


// Вызвать нативную функцию Thread.*
sap_byte sapvm_call_native_Thread(sap_vm *vm, sap_uint Native_Hash, sap_thread *thr, sap_frame *f, sap_object *obj, sap_byte params_len, sap_byte *params)
{
    if (Native_Hash==0x8a719d30)
    {
	// native byte Thread.__fork(byte[] name);
	// Создать новую нитку
	sap_array *name=(sap_array*)R_REF(params+0);
	
	// Проверим имя
	if (!name) return SAP_FAULT_NULL_REF;
	if (name->ID != 0x8100) return SAP_FAULT_BAD_SPEC;
	
	// Проверим стек - в нем должно быть 4 байта (this)
	if ( (f->SP!=4) || (R_INT(f->stack+0)==0) )
	{
	    //printf("Incorrect stack state for calling Thread.__fork() !\n");
	    return SAP_FAULT_STACK;
	}
	
	// Создаем нитку
	sap_thread *t=(sap_thread*)ALLOC(sizeof(sap_thread), SAP_HEAP_THREAD);
	if (!t) return SAP_FAULT_NO_MEMORY;	// нет памяти !
	sap_uint l=name->length;
	if (l>sizeof(t->name)-1) l=sizeof(t->name)-1;
	memcpy(t->name, name->data.b, l);
	t->name[l]=0;
	t->sleep_count=0;
	t->unlock_obj=0;
	t->next=(sap_thread*)0;
	
	// Клонируем фрейм
	t->frame=(sap_frame*)ALLOC(sizeof(sap_frame), SAP_HEAP_FRAME);
	if (!t->frame)
	{
	    // Ошибка
	    FREE((void*)t);
	    return SAP_FAULT_NO_MEMORY;
	}
	t->frame->stack=(sap_byte*)ALLOC(f->stack_size, SAP_HEAP_STACK);
	if (!t->frame->stack)
	{
	    // Ошибка
	    FREE((void*)t->frame);
	    FREE((void*)t);
	    return SAP_FAULT_NO_MEMORY;
	}
	memcpy(t->frame->stack, f->stack, f->stack_size);
	t->frame->stack_size=f->stack_size;
	t->frame->PC=f->PC;
	t->frame->SP=f->SP;
	t->frame->Code_AT=f->Code_AT;
	t->frame->Method_ID=f->Method_ID;
	t->frame->prev=(sap_frame*)0;
	
	// Убираем нативный фрейм
	t->native_frame=0;
	
	// Забываем имя нитки
	sapvm_dec_ref(vm, (sap_object*)name);
	
	// Увеличиваем счетчик ссылок на this
	sapvm_inc_ref(vm, R_REF(f->stack+0));
	
	// Раскладываем результаты выполнения
	
	// calling_thread: return 0;
	W_INT(f->stack + f->SP, 0);
	f->SP+=4;
	
	// new_thread: return 1;
	W_INT(t->frame->stack + t->frame->SP, 1);
	t->frame->SP+=4;
	
	// Добавляем нитку к списку ниток
	while (thr->next) thr=thr->next;
	thr->next=t;
	
	// Нет ошибки
	return 0;
    } else
    if (Native_Hash==0x8d5ba3fd)
    {
	// native static void Thread.sleep(int ms);
	thr->sleep_count=R_INT(params+0);
	if (thr->sleep_count < 0) thr->sleep_count=0;
	
	// return 0;
	W_INT(f->stack + f->SP, 0);
	f->SP+=4;
	
	// Нет ошибки
	return (thr->sleep_count > 0) ? -1 : 0;	// переключаем задачу, если засыпаем
    } else
    if (Native_Hash==0xe810eeb3)
    {
	// native static void Thread.yield();
	
	// return 0;
	W_INT(f->stack + f->SP, 0);
	f->SP+=4;
	
	// Нет ошибки
	return -1;	// надо переключиться в другую нитку
    } else
    
    // Нет обработчика
    return SAP_FAULT_NATIVE_NOT_FOUND;
}
