#include "sap_native.h"

#include "port.h"
#include "data.h"
#include "vm.h"
#include "sapvm_opts.h"

#include <stdio.h>
#include <string.h>


// Вызвать нативную функцию IPC.*
sap_byte sapvm_call_native_IPC(sap_vm *vm, sap_uint Native_Hash, sap_thread *thr, sap_frame *f, sap_object *obj, sap_byte params_len, sap_byte *params)
{
    if (Native_Hash==0x06ecd5d7)
    {
	// native static byte IPC.tryLock(Object o);
	sap_object *o=R_REF(params+0);
	if (!o) return SAP_FAULT_NULL_REF;
	
        // Проверяем - заблокирован ли объект
	sap_byte ok=1;
        sap_lock *l=vm->locks;
        while (l)
        {
            if (l->obj == o)
            {
                // Объект заблокирован
		
                // Проверим - какой ниткой
                if (l->thr != thr)
                {
                    // Не нашей ниткой - придется ждать
		    ok=0;
                } else
                {
                    // Блокировка нашей ниткой. Просто добавляем счетчик блокировок
                    l->count++;
                }
                break;
            }
            l=l->next;
        }
	
        if ( (ok) && (!l) )
        {
            // Объект никем не заблокирован. Надо создать новую блокировку
            l=(sap_lock*)ALLOC(sizeof(sap_lock), SAP_HEAP_VM);
            if (!l)
            {
                // Не получилось выделить блокировку
                vm->fault_cause=SAP_FAULT_NO_MEMORY;
                return SAP_FAULT;
            }
            l->obj=o;
            l->thr=thr;
            l->count=1;
            l->next=vm->locks;
            vm->locks=l;
        }
	
	// Забываем объект
	sapvm_dec_ref(vm, o);
	
	// return ok;
	W_INT(f->stack + f->SP, ok);
	f->SP+=4;
	
	// Нет ошибки
	return 0;
    } else
    if (Native_Hash==0x5b0bbae9)
    {
        // native static void IPC.lock(Object o);
	sap_object *o=R_REF(params+0);
	if (!o) return SAP_FAULT_NULL_REF;
	
        // Проверяем - заблокирован ли объект
        sap_lock *l=vm->locks;
        while (l)
        {
            if (l->obj == o)
            {
                // Объект заблокирован
		
                // Проверим - какой ниткой
                if (l->thr != thr)
                {
                    // Не нашей ниткой - придется ждать
                    thr->unlock_obj=o;
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
            l->obj=o;
            l->thr=thr;
            l->count=1;
            l->next=vm->locks;
            vm->locks=l;
        }
	
	// Забываем объект
	sapvm_dec_ref(vm, o);
	
	// return 0;
	W_INT(f->stack + f->SP, 0);
	f->SP+=4;
	
	// Нет ошибки
	return thr->unlock_obj ? -1 : 0;	// просим переключение задач при блокировке
    } else
    if (Native_Hash==0x46180de7)
    {
	// native static void IPC.unlock(Object o);
	sap_object *o=R_REF(params+0);
	if (!o) return SAP_FAULT_NULL_REF;
	
        // Снимаем блокировку
	sap_int rv=0;
        sap_lock *prev=0, *l=vm->locks;
        while (l)
        {
            if (l->obj == o)
            {
                // Нашли заблокированный объект
                l->count--;
                if (l->count <= 0)
                {
                    // Надо удалить блокировку
		    
                    // Проверяем - если какая-то нитка ждала этот объект - просыпаем ее и блокируем объект на новую нитку, иначе удаляем блокиров
                    sap_thread *t=vm->threads;
                    while (t)
                    {
                        if (t->unlock_obj == o)
                        {
                            t->unlock_obj=0;
                            l->thr=t;
                            l->count=1;
                            rv=-1;       // сделаем принудительное переключение ниток
                            break;
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
	sapvm_dec_ref(vm, o);
	
	// return 0;
	W_INT(f->stack + f->SP, 0);
	f->SP+=4;
	
	// Нет ошибки
	return rv;
    } else
    
    // Нет обработчика
    return SAP_FAULT_NATIVE_NOT_FOUND;
}
