#include "sapvm.h"

#include "types.h"
#include "port.h"
#include "data.h"
#include "vm.h"
#include "sapvm_opts.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*
 * Создать новую виртуальную машину
 */
SAP_VM sapvm_init(int argc, char **argv)
{
    sap_vm *vm;
    
    // Проверим сигнатуру
    {
	char signature[9];
	
	RBUF(0, (sap_byte*)signature, 8);
	signature[8]=0;
	
	if (strcmp(signature, "#!sapvm\n"))
	{
	    // Неверная сигнатура
	    SAPVM_DEBUG("sapvm: Incorrect signature !\r\n");
	    return (sap_vm*)0;
	}
    }
    
    
    // Выделяем память под рантайм
    {
	vm=(sap_vm*)ALLOC(sizeof(sap_vm), SAP_HEAP_VM);
	if (!vm) return (sap_vm*)0;
    }
    
    
    // Параметры запуска
    vm->argc=argc;
    vm->argv=argv;
    
    
    // Читаем кол-во классов и методов
    vm->n_classes=RS(8);
    vm->n_methods=RS(10);
    
    
    // Создаем статические поля
    {
	sap_short static_len;
	
	static_len=RS(12);
	if (static_len > 0)
	{
	    // Есть статические поля
	    sap_short i;
	    
	    //SAPVM_DEBUG("\tALLOC static_fields");
	    vm->static_fields=(sap_byte*)ALLOC(static_len, SAP_HEAP_VM);
	    if (!vm->static_fields)
	    {
		// Не получилось выделить память под статические переменные
		FREE((void*)vm);
		return (sap_vm*)0;
	    }
	    
	    // Инитим статические переменные нулями
	    for (i=0; i<static_len; i++)
		vm->static_fields[i]=0;
	} else
	{
	    // Нет статических полей
	    vm->static_fields=(sap_byte*)0;
	}
	
	// Читаем кол-во статических ссылок
	vm->n_static_refs=RS(14);
    }
    
    
    // Получаем расположение секций
    {
	vm->Boot_AT=RI(16);
	vm->Classes_AT=RI(20);
	vm->InstanceOfTab_AT=RI(24);
	vm->VirtualTab_AT=RI(28);
	vm->InterfaceBindTab_AT=RI(32);
	vm->RefFieldsTab_AT=RI(36);
	vm->Methods_AT=RI(40);
	vm->Code_AT=RI(44);
	vm->Debug_AT=RI(48);
	vm->LineNumbers_AT=RI(52);
	vm->DebugStringPool_AT=RI(56);
    }
    
    
#ifdef SAPVM_CACHE
    // Предкэшируем секции для ускорения выполнения
    {
	// Получаем размеры секций
	sap_int Classes_Size		= 												     vm->InstanceOfTab_AT - vm->Classes_AT;
	sap_int InstanceOfTab_Size	= 										 vm->VirtualTab_AT - vm->InstanceOfTab_AT;
	sap_int VirtualTab_Size		= 						       vm->InterfaceBindTab_AT - vm->VirtualTab_AT;
	sap_int InterfaceBindTab_Size	= 				 vm->RefFieldsTab_AT - vm->InterfaceBindTab_AT;
	sap_int RefFieldsTab_Size	= 		vm->Methods_AT - vm->RefFieldsTab_AT;
	sap_int Methods_Size		= vm->Code_AT - vm->Methods_AT;
	
	// Создаем кэш для всех секций
	SAPVM_DEBUG("Allocating %d bytes for cache\r\n",
	    Classes_Size + InstanceOfTab_Size + VirtualTab_Size + InterfaceBindTab_Size + RefFieldsTab_Size + Methods_Size );
	
	vm->Classes		= (sap_byte*)ALLOC(Classes_Size,		SAP_HEAP_VM);
	vm->InstanceOfTab	= (sap_byte*)ALLOC(InstanceOfTab_Size,		SAP_HEAP_VM);
	vm->VirtualTab		= (sap_byte*)ALLOC(VirtualTab_Size,		SAP_HEAP_VM);
	vm->InterfaceBindTab	= (sap_byte*)ALLOC(InterfaceBindTab_Size,	SAP_HEAP_VM);
	vm->RefFieldsTab	= (sap_byte*)ALLOC(RefFieldsTab_Size,		SAP_HEAP_VM);
	vm->Methods		= (sap_byte*)ALLOC(Methods_Size,		SAP_HEAP_VM);
	if ( (!vm->Classes) ||
	     (!vm->InstanceOfTab) ||
	     (!vm->VirtualTab) ||
	     (!vm->InterfaceBindTab) ||
	     (!vm->RefFieldsTab) ||
	     (!vm->Methods) )
	{
	    // Ошибка выделения кэша !
	    SAPVM_DEBUG("sapvm: Can't allocate cache !\r\n");
	    if (vm->Classes)		FREE((void*)vm->Classes);
	    if (vm->InstanceOfTab)	FREE((void*)vm->InstanceOfTab);
	    if (vm->VirtualTab)		FREE((void*)vm->VirtualTab);
	    if (vm->InterfaceBindTab)	FREE((void*)vm->InterfaceBindTab);
	    if (vm->RefFieldsTab)	FREE((void*)vm->RefFieldsTab);
	    if (vm->Methods)		FREE((void*)vm->Methods);
	    
	    if (vm->static_fields)	FREE((void*)vm->static_fields);
	    FREE((void*)vm);
	    return (sap_vm*)0;
	}
	
	// Загружаем секции
	RBUF(vm->Classes_AT,		vm->Classes,		Classes_Size);
	RBUF(vm->InstanceOfTab_AT,	vm->InstanceOfTab,	InstanceOfTab_Size);
	RBUF(vm->VirtualTab_AT,		vm->VirtualTab,		VirtualTab_Size);
	RBUF(vm->InterfaceBindTab_AT,	vm->InterfaceBindTab,	InterfaceBindTab_Size);
	RBUF(vm->RefFieldsTab_AT,	vm->RefFieldsTab,	RefFieldsTab_Size);
	RBUF(vm->Methods_AT,		vm->Methods,		Methods_Size);
    }
#endif
    
    // Получаем идентификаторы стандартных классов
    vm->clsid_Object=sapvm_find_class_id(vm, "Object");
    vm->clsid_String=sapvm_find_class_id(vm, "String");
    vm->clsid_StringBuffer=sapvm_find_class_id(vm, "StringBuffer");
    vm->clsid_DateTime=sapvm_find_class_id(vm, "DateTime");
    
    // Создаем нитку с кодом загрузки
    vm->threads=(sap_thread*)ALLOC(sizeof(sap_thread), SAP_HEAP_THREAD);
    if (!vm->threads)
    {
	// Не получилось выделить память под нитку
	if (vm->static_fields) FREE((void*)vm->static_fields);
	FREE((void*)vm);
	return (sap_vm*)0;
    }
    strcpy(vm->threads->name, "main");
    vm->threads->sleep_count=0;
    vm->threads->unlock_obj=0;
    vm->threads->next=(sap_thread*)0;	// следующей нитки нету
    
    // Создаем фрейм для кода загрузки
    vm->threads->frame=sapvm_create_frame(vm->Boot_AT, -1, (sap_frame*)0);
    if (!vm->threads->frame)
    {
	// Не получилось создать загрузочный фрейм
	FREE((void*)vm->threads);
	if (vm->static_fields) FREE((void*)vm->static_fields);
	FREE((void*)vm);
	return (sap_vm*)0;
    }
    
    // Ставим указатель this
    W_INT(vm->threads->frame->stack+0, 0);
    
    // Убираем нативный фрейм
    vm->threads->native_frame=0;
    
    // Инитим блокировки
    vm->locks=0;
    
    // Запоминаем текущее время
    vm->last_time_counter=TIME_COUNTER();
    
    // Снимаем ошибку виртуальной машины
    vm->fault_flag=0;
    
#ifdef SAPVM_STATS
    // Сбрасываем статистику
    {
	sap_int i;
	for (i=0; i<256; i++)
	    vm->bytecodes_executed[i]=0;
	vm->bytecodes_executed_total=0;
    }
#endif
    
    // Все готово
    return vm;
}


/*
 * Удалить виртуальную машину
 */
void sapvm_done(SAP_VM vm)
{
#warning TODO: подчищать все, если было некорректное завершение
    
#ifdef SAPVM_CACHE
    // Удаляем кэш
    if (vm->Classes)		FREE((void*)vm->Classes);
    if (vm->InstanceOfTab)	FREE((void*)vm->InstanceOfTab);
    if (vm->VirtualTab)		FREE((void*)vm->VirtualTab);
    if (vm->InterfaceBindTab)	FREE((void*)vm->InterfaceBindTab);
    if (vm->RefFieldsTab)	FREE((void*)vm->RefFieldsTab);
    if (vm->Methods)		FREE((void*)vm->Methods);
#endif
    
    if (vm->static_fields)
    {
	// Удаляем статические ссылки
	int i;
	for (i=0; i<vm->n_static_refs; i++)
	{
	    sap_object *obj=R_REF(vm->static_fields + i*4);
	    sapvm_dec_ref(vm, obj);
	}
	FREE((void*)vm->static_fields);
    }
    FREE((void*)vm);
}


/*
 * Создать фрейм
 */
sap_frame* sapvm_create_frame(sap_int Frame_AT, sap_short Method_ID, sap_frame *prev)
{
    // Создаем фрейм
    sap_frame *f=(sap_frame*)ALLOC(sizeof(sap_frame), SAP_HEAP_FRAME);
    if (!f) return (sap_frame*)0;
    
    // Создаем стек
    f->stack_size=RS(Frame_AT+0);
    f->stack=(sap_byte*)ALLOC(f->stack_size, SAP_HEAP_STACK);
    if (!f->stack)
    {
	// Не удалось выделить стек
	FREE((void*)f);
	return (sap_frame*)0;
    }
    
    // Читаем начальные настройки стека
    f->SP=RS(Frame_AT+2);
    
    // Читаем размер кода
    f->Code_Size=(sap_ushort)RS(Frame_AT+4);
    
    // Заполняем остальные поля
    f->PC=0;
    f->Code_AT=Frame_AT+6;
    f->Method_ID=Method_ID;
    f->prev=prev;
    
    // Все нормально
    return f;
}



/*
 * Подготовиться к началу выполнения
 */
void sapvm_prepare_execution(SAP_VM vm)
{
    // Запоминаем текущее время
    vm->last_time_counter=TIME_COUNTER();
}


/*
 * Выполнить шаг виртуальной машины.
 * Тут выполняется по STEP_N_COMMANDS команд из каждой нитки.
 */
sap_byte sapvm_execute_step(SAP_VM vm)
{
    // Проверим - может быть была ошибка
    if (vm->fault_flag) return SAP_FAULT;
    
    // Получаем, сколько времени прошло с прошлого исполнения
    sap_uint tm=TIME_COUNTER();
    sap_uint n=tm-vm->last_time_counter;
    vm->last_time_counter=tm;
    
    // Выполняем все нити
    sap_thread *thread=vm->threads;
    sap_thread *prev=0;	// предыдущая нить, на случай умирания нитки
    while (thread)
    {
	// Проверяем счетчик сна
	if (thread->sleep_count > n)
	    thread->sleep_count-=n; else
	    thread->sleep_count=0;
	
	sap_byte rv;
	if ( (thread->sleep_count > 0) ||
	     (thread->unlock_obj) )
	    rv=SAP_OK; else
	    rv=sapvm_execute_thread(vm, thread);
	sap_thread *next=thread->next;
	
	switch (rv)
	{
	    case SAP_OK:
		// Все нормально
		prev=thread;
		break;
	    
	    case SAP_THREAD_EXIT:
		// Выход из нитки. Правим цепочку ниток
		if (prev)
		{
		    prev->next=thread->next;
		} else
		{
		    vm->threads=thread->next;
		}
		// Удаляем текущую нитку
		FREE((void*)thread);
		break;
	    
	    case SAP_FAULT:
	    default:
		// Ошибка времени выполнения
		vm->fault_flag=1;
		vm->fault_thread=thread;
		return SAP_FAULT;
	}
	
	// Выбираем следующую нитку
	thread=next;
    }
    
    // Проверим - может быть программа завершилась
    if (!vm->threads) return SAP_PROGRAM_EXIT;
    
    // Все нормально
    return SAP_OK;
}


/*
 * Получить время, которое может спать виртулаьная машина
 */
sap_int sapvm_get_sleep_time(SAP_VM vm)
{
    sap_thread *thread=vm->threads;
    sap_uint sleep_time=thread->sleep_count;
    while ( (thread) && (sleep_time > 0) )
    {
	if (thread->sleep_count < sleep_time)
	    sleep_time=thread->sleep_count;
	thread=thread->next;
    }
    return sleep_time;
}


/*
 * Выполнять программу целиком
 */
sap_byte sapvm_execute(SAP_VM vm)
{
    sap_byte rv;
    
    do
    {
	rv=sapvm_execute_step(vm);
	if (rv==SAP_OK)
	{
	    // Проверим - может быть можно поспать
	    sap_int sleep_time=sapvm_get_sleep_time(vm);
	    if (sleep_time > 0) SLEEP(sleep_time);
	}
    } while (rv==SAP_OK);
    
    return rv;
}


// Вывести сообщение об ошибке
void sapvm_show_error(SAP_VM vm)
{
    // Проверим - а есть ли ошибка ?
    if (!vm->fault_flag) return;
    
    SAPVM_PRINTF("\tSAP: ");
    switch (vm->fault_cause)
    {
	case SAP_FAULT_NO_MEMORY:
	    SAPVM_PRINTF("out of memory");
	    break;
	
	case SAP_FAULT_STACK:
	    SAPVM_PRINTF("stack fault");
	    break;
	
	case SAP_FAULT_NULL_REF:
	    SAPVM_PRINTF("null reference");
	    break;
	
	case SAP_FAULT_ARRAY_INDEX:
	    SAPVM_PRINTF("array index out of range");
	    break;
	
	case SAP_FAULT_ARRAY_LENGTH:
	    SAPVM_PRINTF("bad array length (<0)");
	    break;
	
	case SAP_FAULT_IFACE_METHOD:
	    SAPVM_PRINTF("interface method not found");
	    break;
	
	case SAP_FAULT_NATIVE_NOT_FOUND:
	    SAPVM_PRINTF("native method not found");
	    break;
	
	case SAP_FAULT_BAD_SPEC:
	    SAPVM_PRINTF("bad type specifier");
	    break;
	
	case SAP_FAULT_CAST:
	    SAPVM_PRINTF("bad cast");
	    break;
	
	case SAP_FAULT_BAD_OPCODE:
	    SAPVM_PRINTF("bad opcode 0x%02x", (sap_ubyte)RB(vm->fault_thread->frame->Code_AT + vm->fault_thread->frame->PC-1));
	    break;
	
	default:
	    SAPVM_PRINTF("unkown fault %d", vm->fault_cause);
	    break;
    }
    SAPVM_PRINTF(" in thread '%s':\r\n", vm->fault_thread->name);
    
    // Проверяем - есть ли отладочная инфа
    if (vm->Debug_AT==0) return;
    
    // Отображаем стек вызовов
    sapvm_show_call_stack(vm, vm->fault_thread->frame);
    SAPVM_PRINTF("\r\n");
}


// Вывести стек вызовов
void sapvm_show_call_stack(sap_vm *vm, sap_frame *f)
{
    while (f)
    {
	if (f->Method_ID==-1)
	{
	    // Это boot
	    SAPVM_PRINTF("\tat <boot> (PC=%d)\r\n",f->PC);
	    return;
	}
	
	
	// Получаем адрес отладки метода
	sap_int MethodDebug_AT=vm->Debug_AT + sizeof(sap_classdebug_storage)*vm->n_classes + sizeof(sap_methoddebug_storage)*f->Method_ID;
	
	// Получим идентификатор класса
	sap_short Class_ID=RS(MethodDebug_AT + 0);
	
	// Получаем адрес отладки класса
	sap_int ClassDebug_AT=vm->Debug_AT + sizeof(sap_classdebug_storage)*Class_ID;
	
	// Получаем номера класса и метода в таблице строк
	sap_short strClass_Name=RS(ClassDebug_AT + 0);
	sap_short strMethod_Name=RS(MethodDebug_AT + 2);
	
	// Получаем положение таблицы строк
	sap_int LineNumbers_AT=vm->LineNumbers_AT + RI(MethodDebug_AT + 4);
	
	
	// Выводим ошибку
	SAPVM_PRINTF("\tat ");
	sapvm_show_StringPool_Item(vm, strClass_Name);
	SAPVM_PRINTF(".");
	sapvm_show_StringPool_Item(vm, strMethod_Name);
	SAPVM_PRINTF(" (");
	
	
	// Выводим информацию об исходнике
	sap_short strSource_fname=RS(LineNumbers_AT);
	LineNumbers_AT+=2;
	sapvm_show_StringPool_Item(vm, strSource_fname);
	SAPVM_PRINTF(":");
	
	// Ищем строку
	sap_short Line=1;
	sap_short size=RS(LineNumbers_AT);
	LineNumbers_AT+=2;
	while (size--)
	{
	    sap_short PC=RS(LineNumbers_AT);
	    LineNumbers_AT+=2;
	    sap_short L=RS(LineNumbers_AT);
	    LineNumbers_AT+=2;
	    
	    if (PC>=f->PC) break;
	    
	    Line=L;
	}
	SAPVM_PRINTF("%d)\r\n", Line);
	
	
	// Берем вызывающего
	f=f->prev;
    }
}


// Вывести строку из таблицы отладочных строк
void sapvm_show_StringPool_Item(sap_vm *vm, sap_short idx)
{
    sap_int AT=vm->DebugStringPool_AT;
    
    // Получаем размер
    sap_short size=RS(AT);
    AT+=2;
    if (idx >= size)
    {
	SAPVM_PRINTF("<error>");
	return;
    }
    
    // Пропускаем лишние строки
    while (idx--)
    {
	sap_ushort l=(sap_ubyte)RB(AT++);
	if (l & 0x80)
	{
	    // Размер 2 байта
	    l=(l & 0x7f) << 8;
	    l|=(sap_ubyte)RB(AT++);
	}
	AT+=l;
    }
    
    // Выводим нашу строку
    sap_ushort l=(sap_ubyte)RB(AT++);
    if (l & 0x80)
    {
	// Размер 2 байта
	l=(l & 0x7f) << 8;
	l|=(sap_ubyte)RB(AT++);
    }
    
    while (l--)
    {
	SAPVM_PRINTF("%c",(char)RB(AT++));
    }
}


// Получить строку из таблицы отладочных строк (выделяется ALLOC-ом)
char* sapvm_get_StringPool_Item(sap_vm *vm, sap_short idx)
{
    sap_int AT=vm->DebugStringPool_AT;
    
    // Получаем кол-во строк
    sap_short size=RS(AT);
    AT+=2;
    if (idx >= size) return 0;
    
    // Пропускаем лишние строки
    while (idx--)
    {
	sap_ushort l=(sap_ubyte)RB(AT++);
	if (l & 0x80)
	{
	    // Размер 2 байта
	    l=(l & 0x7f) << 8;
	    l|=(sap_ubyte)RB(AT++);
	}
	AT+=l;
    }
    
    // Читаем строку
    sap_ushort l=(sap_ubyte)RB(AT++);
    if (l & 0x80)
    {
	// Размер 2 байта
	l=(l & 0x7f) << 8;
	l|=(sap_ubyte)RB(AT++);
    }
    
    char *str=(char*)ALLOC(l+1, SAP_HEAP_VM);
    if (!str) return 0;
    
    char *ss=str;
    while (l--)
    {
	(*ss++)=(char)RB(AT++);
    }
    (*ss)=0;
    
    return str;
}


// Получить идентификатор класса по его имени
sap_ubyte sapvm_find_class_id(sap_vm *vm, const char *name)
{
    sap_int i;
    
    for (i=0; i<vm->n_classes; i++)
    {
	// Получаем адрес отладки класса
	sap_int ClassDebug_AT=vm->Debug_AT + sizeof(sap_classdebug_storage)*i;
	
	// Получаем номера класса в таблице строк
	sap_short strClass_Name=RS(ClassDebug_AT + 0);
	
	char *cls=sapvm_get_StringPool_Item(vm, strClass_Name);
	if (!cls) continue;
	
	if (!strcmp(name, cls))
	{
	    FREE((void*)cls);
	    return i;
	}
	FREE((void*)cls);
    }
    
    return 0;
}


// Получить имя класса по его идентификатору (выделяется ALLOC-ом)
char* sapvm_find_class_name(sap_vm *vm, sap_ubyte id)
{
    // Получаем адрес отладки класса
    sap_int ClassDebug_AT=vm->Debug_AT + sizeof(sap_classdebug_storage)*id;
    
    // Получаем номера класса в таблице строк
    sap_short strClass_Name=RS(ClassDebug_AT + 0);
    
    return sapvm_get_StringPool_Item(vm, strClass_Name);
}


// Проверить - является ли объект потомком класса, определенного спецификатором
sap_byte sapvm_is_instanceof(sap_vm *vm, sap_object *obj, sap_ushort spec)
{
    // Получаем спецификатор объекта
    sap_ushort obj_spec=obj->ID;
    
    // Делаем проверку типа
    sap_byte ok=0;
    
    // Убираем лишние размерности
    if ( ((spec >> 8) & 0x0f) > ((obj_spec >> 8) & 0x0f) )
    {
	// Нельзя кастить к большей размерности
    } else
    {
	// Убираем лишние размерности
	sap_byte dim=(spec >> 8) & 0x0f;
	spec=spec & 0xf0ff;
	obj_spec=(obj_spec & 0xf0ff) | ((((obj_spec >> 8) & 0x0f) - dim) << 8);
	
	if ( (spec & 0xf000) == 0xb000 )
	{
	    // Приведение к классу
	    
	    // К Object можно привести либо массив, либо другой класс
	    if (spec == 0xb000)
	    {
		// Приведение к Object
		if ( ( (obj_spec & 0xf000) == 0xb000 ) ||
		     ( (obj_spec & 0x0f00) != 0 ) )
		{
		    ok=1;
		}
	    } else
	    {
		// Приведение к другому классу - можно приводить только классы
		if ( (obj_spec & 0xf000) == 0x0000 )
		{
		    // Приводим класс
		    
		    // Получаем адрес класса
		    sap_int Class_Offs=sizeof(sap_class_storage)*(obj->ID & 0x00ff);
		    
		    // Получаем адрес таблицы InstanceOf
#ifndef SAPVM_CACHE
		    sap_int InstanceOfTab_Offs=RS(vm->Classes_AT + Class_Offs + 2);
#else
		    sap_int InstanceOfTab_Offs=R_SHORT(vm->Classes + Class_Offs + 2);
#endif
		    
		    // Ищем
		    sap_short id;
		    do
		    {
#ifndef SAPVM_CACHE
			id=RS(vm->InstanceOfTab_AT + InstanceOfTab_Offs);
#else
			id=R_SHORT(vm->InstanceOfTab + InstanceOfTab_Offs);
#endif
			//printf("id=%04x\r\n",id);
			InstanceOfTab_Offs+=2;
			if ( id == (spec & 0x00ff) )
			{
			    ok=1;
			    break;
			}
		    } while (id!=0);
		}
	    }
	} else
	{
	    // Приведение к базовому типу. Базовый тип должен точно сходиться и размерность тоже
	    if (spec == obj_spec)
	    {
		ok=1;
	    }
	}
    }
    
    return ok;
}


// Преобразовать byte[] в char* (выделяется с помощью ALLOC)
char* sapvm_byte_array_to_string(sap_array *arr)
{
    if (!arr) return 0;
    if (arr->ID != 0x8100) return 0;
    
    char *str=(char*)ALLOC(arr->length+1, SAP_HEAP_VM);
    if (str)
    {
        memcpy(str, arr->data.b, arr->length);
        str[arr->length]=0;
    }
    
    return str;
}
