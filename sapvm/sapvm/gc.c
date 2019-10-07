#include "gc.h"

#include "sapvm.h"
#include "port.h"
#include "data.h"
#include "sapvm_opts.h"


// Удалить объект obj и убрать за ним мусор
void sapvm_gc(sap_vm *vm, sap_object *obj)
{
    //printf("GC obj 0x%08x (ID=0x%04x)\n", (unsigned int)obj, obj->ID);
    // Проверим, что это - объект или массив
    if (obj->ID & 0x8000)
    {
	// Это массив
	
	// Вложенное удаление требуется только для массива ссылок
	if ( ( ((obj->ID >> 12) & 0x07) == 3 ) ||
	     ( ((obj->ID >> 8) & 0x0f) > 1 ) )
	{
	    // Это массив ссылок
	    sap_int i;
	    sap_array *arr=(sap_array*)obj;
	    
	    //printf("GC array at 0x%08x (ID=0x%04x) length=%d\n", (unsigned int)arr, obj->ID, arr->length);
	    
	    for (i=0; i<arr->length; i++)
	    {
		sap_object *o=arr->data.r[i];
		//printf("  arr[%d]=0x%08x\n", i, (unsigned int)o);
		if (o)
		{
		    o->ref_count--;
		    if (o->ref_count <= 0) sapvm_gc(vm, o);
		}
	    }
	}
    } else
    {
	// Это объект
	
	// Получаем адрес класса
	sap_int Class_Offs=sizeof(sap_class_storage)*obj->ID;
	
	// Получаем адрес таблицы полей-ссылок этого класса
#ifndef SAPVM_CACHE
	sap_int RefFieldsTab_Offs=RS(vm->Classes_AT + Class_Offs + 8);
#else
	sap_int RefFieldsTab_Offs=R_SHORT(vm->Classes + Class_Offs + 8);
#endif
	
	// Читаем таблицу полей
	sap_short addr;
	do
	{
	    // Читаем адрес
#ifndef SAPVM_CACHE
	    addr=RS(vm->RefFieldsTab_AT + RefFieldsTab_Offs);
#else
	    addr=R_SHORT(vm->RefFieldsTab + RefFieldsTab_Offs);
#endif
	    RefFieldsTab_Offs+=2;
	    
	    if (addr!=-1)
	    {
		// Проверяем ссылку
		sap_object *o=R_REF(obj->fields + addr);
		if (o)
		{
		    o->ref_count--;
		    if (o->ref_count <= 0) sapvm_gc(vm, o);
		}
	    }
	} while (addr!=-1);
    }
    
    // Все ссылки подчищены. Убиваем объект
    FREE((void*)obj);
}
