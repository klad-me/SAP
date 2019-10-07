#ifndef SAPVM_H
#define SAPVM_H


#include "types.h"
#include "sapvm_opts.h"
#include <pt.h>


// Статусы
enum
{
    SAP_OK=0,
    SAP_THREAD_EXIT,
    SAP_PROGRAM_EXIT,
    SAP_FAULT,
};


// Ошибки
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


// Объект
typedef struct sap_object
{
    sap_ushort ID;
    sap_short ref_count;
    
    sap_byte fields[1];
} __attribute__ ((packed)) sap_object;


// Массив
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


// Фрейм
typedef struct sap_frame
{
    // Регистры виртуальной машины: счетчик команд и указатель стека
    sap_short PC, SP;
    
    // Стек
    sap_byte *stack;
    sap_short stack_size;
    
    // Расположение и размер кода
    sap_int Code_AT;
    sap_ushort Code_Size;
    
    // Идентификатор выполняемого метода (чтобы выводить нормальное сообщение об ошибке)
    sap_short Method_ID;
    
    // Предыдущий фрейм (из которого был вызван этот фрейм)
    struct sap_frame *prev;
} __attribute__ ((packed)) sap_frame;


// Нативный фрейм (блокирующий нативный вызов, сделанный на proto-threads)
// Нативный фрейм может менять sleep_count для засыпания
struct sap_vm;
struct sap_thread;
typedef PT_THREAD( (*sap_native_frame_f)(struct sap_vm*, struct sap_thread*) );
typedef struct sap_native_frame
{
    struct pt pt;		// нитка ProtoThread
    sap_object *obj;		// объект вызова
    sap_native_frame_f code;	// нативный код нитки
    sap_uint Native_Hash;	// идентификатор метода
    sap_byte result;		// результат выполнения 0=OK, другое - SAP_FAULT_*
    void *data;			// данные для работы нитки
} sap_native_frame;


// Макрос для создания нативного фрейма
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



// Нитка
typedef struct sap_thread
{
    // Текущий исполняемый фрейм
    sap_frame *frame;
    
    // Нативный фрейм
    sap_native_frame *native_frame;
    
    // Имя нитки
    char name[8];
    
    // Сколько времени осталось нитке спать
    sap_uint sleep_count;
    
    // Разблокировку какого объекта ждет нитка
    sap_object* unlock_obj;
    
    // Указатель на следующую нитку в списке
    struct sap_thread *next;
} __attribute__ ((packed)) sap_thread;


// Блокировка
typedef struct sap_lock
{
    sap_object *obj;
    sap_thread *thr;
    sap_int count;
    
    struct sap_lock *next;
} __attribute__ ((packed)) sap_lock;


// Виртуальная машина
typedef struct sap_vm
{
    // Параметры программы
    int argc;
    char **argv;
    
    // Кол-во классов
    sap_short n_classes;
    
    // Кол-во методов
    sap_short n_methods;
    
    // Расположение секций
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
    // Загруженные в ОЗУ секции для ускорения выполнения
    sap_byte *Classes;
    sap_byte *InstanceOfTab;
    sap_byte *VirtualTab;
    sap_byte *InterfaceBindTab;
    sap_byte *RefFieldsTab;
    sap_byte *Methods;
#endif
    
    // Идентификаторы стандартных классов
    sap_ubyte clsid_Object;
    sap_ubyte clsid_String;
    sap_ubyte clsid_StringBuffer;
    sap_ubyte clsid_DateTime;
    
    // Ошибка
    sap_byte fault_flag;	// флаг
    sap_thread *fault_thread;	// нитка, в которой была ошибка
    sap_byte fault_cause;	// причина ошибки
    
    // Статические поля
    sap_byte *static_fields;
    sap_short n_static_refs;
    
    // Нитки (связанный список)
    sap_thread *threads;
    
    // Блокировки
    sap_lock *locks;
    
    // Последнее значение счетчика времени
    sap_uint last_time_counter;
    
#ifdef SAPVM_STATS
    // Статистика
    sap_uint bytecodes_executed_total;
    sap_uint bytecodes_executed[256];
#endif
} __attribute__ ((packed)) sap_vm;
typedef sap_vm* SAP_VM;


// Секция Class
typedef struct sap_class_storage
{
    sap_short fields_size;
    sap_short InstanceOfTab_AT;
    sap_short VirtualTab_AT;
    sap_short InterfaceBindTab_AT;
    sap_short RefFieldsTab_AT;
} __attribute__ ((packed)) sap_class_storage;


// Секция Method
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


// Секция ClassDebug
typedef struct sap_classdebug_storage
{
    sap_short strName;
} __attribute__ ((packed)) sap_classdebug_storage;


// Секция MethodDebug
typedef struct sap_methoddebug_storage
{
    sap_short ParentClass_ID;
    sap_short strName;
    sap_int   LineNumbers_AT;
} __attribute__ ((packed)) sap_methoddebug_storage;



// Создать виртуальную машину
SAP_VM sapvm_init(int argc, char **argv);

// Удалить виртуальную машину
void sapvm_done(SAP_VM vm);

// Создать фрейм
sap_frame* sapvm_create_frame(sap_int Frame_AT, sap_short Method_ID, sap_frame *prev);

// Подготовиться к началу выполнения
void sapvm_prepare_execution(SAP_VM vm);

// Сделать шаг выполнения (всех нитей)
sap_byte sapvm_execute_step(SAP_VM vm);

// Получить время, которое может виртуальная машина спать
sap_int sapvm_get_sleep_time(SAP_VM vm);

// Выполнять программу до завершения или ошибки
sap_byte sapvm_execute(SAP_VM vm);

// Вывести сообщение об ошибке
void sapvm_show_error(SAP_VM vm);

// Вывести стек вызовов
void sapvm_show_call_stack(sap_vm *vm, sap_frame *f);

// Вывести строку из таблицы отладочных строк
void sapvm_show_StringPool_Item(sap_vm *vm, sap_short idx);

// Получить строку из таблицы отладочных строк (выделяется ALLOC-ом)
char* sapvm_get_StringPool_Item(sap_vm *vm, sap_short idx);

// Получить идентификатор класса по его имени
sap_ubyte sapvm_find_class_id(sap_vm *vm, const char *name);

// Получить имя класса по его идентификатору (выделяется ALLOC-ом)
char* sapvm_find_class_name(sap_vm *vm, sap_ubyte id);

// Проверить - является ли объект потомком класса, определенного спецификатором
sap_byte sapvm_is_instanceof(sap_vm *vm, sap_object *obj, sap_ushort spec);



// Преобразовать byte[] в char* (выделяется с помощью ALLOC)
char* sapvm_byte_array_to_string(sap_array *arr);




#endif
