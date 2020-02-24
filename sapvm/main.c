#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "sapvm/sapvm.h"
#include "sapvm/vm.h"
#include "sapvm/port.h"
#include "sapvm/opcodes.h"
#include "sap_native_File.h"
#include "sap_native_Socket.h"


//#define DEBUG_HEAP


#ifdef DEBUG_HEAP
    #define LIST_N	100000
    void* known_obj[LIST_N];
    sap_byte obj_types[LIST_N];
#endif


unsigned char sapexe[256000];
int n_alloc=0, n_free=0;


void sapvm_read_bytes(sap_int at, sap_byte *buf, sap_int size)
{
    memcpy(buf, sapexe+at, size);
}


sap_byte sapvm_read_byte(sap_int at)
{
    return sapexe[at];
}


sap_short sapvm_read_short(sap_int at)
{
    sap_short tmp;
    sapvm_read_bytes(at, (sap_byte*)&tmp, 2);
    return tmp;
}


sap_int sapvm_read_int(sap_int at)
{
    sap_int tmp;
    sapvm_read_bytes(at, (sap_byte*)&tmp, 4);
    return tmp;
}


void sapvm_setup_cache(sap_ushort magic, sap_ushort size)
{
}


const char *types[]=
{
    "VM",
    "THREAD",
    "FRAME",
    "STACK",
    "OBJECT",
    "ARRAY",
};

void* sapvm_alloc(sap_int size, sap_byte what)
{
    sap_byte *ptr=(sap_byte*)malloc(size+3);
    
    memset((void*)ptr, 0, size+3);
    
    ptr[0]=what;
    ptr[1]=size >> 8;
    ptr[2]=size & 0xff;
    
    //printf("\talloc(%d) for %s (0x%08x)\n",size,types[what],(int)(ptr+3));
    n_alloc++;
    
#ifdef DEBUG_HEAP
    int i;
    for (i=0; i<LIST_N; i++)
    {
	if (!known_obj[i])
	{
	    known_obj[i]=(void*)ptr;
	    obj_types[i]=what;
	    break;
	}
    }
    if (i==LIST_N) printf("Heap overflow !!!\n");
#endif
    
    return (void*)(ptr+3);
}


void sapvm_free(void *ptr)
{
    int ok=1;
    sap_byte *bptr=(sap_byte*)ptr;
    bptr-=3;
#ifdef DEBUG_HEAP
    int i;
    ok=0;
    for (i=0; i<LIST_N; i++)
    {
	if (known_obj[i]==(void*)bptr)
	{
	    ok=1;
	    known_obj[i]=0;
	    break;
	}
    }
#endif
    if (ok)
    {
	//printf("\tfree(%d) for %s at 0x%08x\n", (((unsigned char)bptr[1])<<8) | (((unsigned char)bptr[2]) & 0xff), types[bptr[0]], (unsigned int)ptr);
	free(bptr);
	n_free++;
    } else
    {
	printf("ERROR !!! FREE TO UNKNOWN ADDRESS 0x%08x\n", (unsigned int)ptr);
    }
}


sap_uint sapvm_get_time_counter(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);

    unsigned long long v;

    v=ts.tv_sec;
    v*=1000;
    v+=ts.tv_nsec/1000000;

    return (sap_uint)(v & 0xffffffff);
}


sap_uint sapvm_get_current_time(void)
{
    return time(NULL);
}


void sapvm_set_current_time(sap_uint ut)
{
}


void sapvm_sleep(sap_int ms)
{
    if (ms > 1000) ms=1000;
    ms*=1000;
    usleep(ms);
}


const char* sapvm_get_property(const char *name)
{
    return 0;
}


void sapvm_set_property(const char *name, const char *value)
{
}


sap_int sapvm_rand(void)
{
    return (sap_int)rand();
}


void sapvm_exit(sap_int code)
{
    exit(code);
}


sap_ushort sapvm_8bit_to_unicode(sap_byte   b)
{
    // Используем cp1251
    sap_ubyte c=(sap_ubyte)b;
    
    if (c<=127) return c; else
    if (c>=0xC0) return 0x410+(c-0xC0); else
    if (c==0xA8) return 0x401; else     // YO
    if (c==0xB8) return 0x451; else     // yo
        return '?';
}


sap_byte   sapvm_unicode_to_8bit(sap_ushort u)
{
    // Используем cp1251
    if (u<=127) return (sap_byte)u; else
    if ( (u>=0x410) && (u<=0x44F) ) return (sap_byte)((u-0x410)+0xC0); else
    if (u==0x401) return (sap_byte)0xA8; else     // YO
    if (u==0x451) return (sap_byte)0xB8; else     // yo
        return (sap_byte)'?';
}


sap_byte sapvm_lower_case(sap_byte b)
{
    sap_ubyte c=(sap_ubyte)b;
    
    if ( (c>='A') && (c<='Z') ) return (sap_byte)(c-'A'+'a'); else
    if ( (c>=0xC0) && (c<=0xDF) ) return (sap_byte)(c-0xC0+0xE0); else
    if (c==0xA8) return (sap_byte)0xB8; else
	return b;
}


sap_byte sapvm_upper_case(sap_byte b)
{
    sap_ubyte c=(sap_ubyte)b;
    
    if ( (c>='a') && (c<='z') ) return (sap_byte)(c-'a'+'A'); else
    if ( (c>=0xE0) && (c<=0xFF) ) return (sap_byte)(c-0xE0+0xC0); else
    if (c==0xB8) return (sap_byte)0xA8; else
	return b;
}


sap_byte sapvm_call_native_User(sap_vm *vm, sap_int Native_Hash, sap_thread *t, sap_frame *f, sap_object *obj, sap_byte params_len, sap_byte *params)
{
    sap_int rv;
    
    rv=sapvm_call_native_File(vm, Native_Hash, t, f, obj, params_len, params);
    if (rv!=SAP_FAULT_NATIVE_NOT_FOUND) return rv;
    
    rv=sapvm_call_native_Socket(vm, Native_Hash, t, f, obj, params_len, params);
    if (rv!=SAP_FAULT_NATIVE_NOT_FOUND) return rv;
    
    return SAP_FAULT_NATIVE_NOT_FOUND;
}


void sapvm_print(sap_byte *buf, sap_int size)
{
    while (size--)
	putchar((char)(*buf++));
    fflush(stdout);
}


#ifdef _WIN32
    #include <windows.h>
#endif


int main(int argc, char **argv)
{
    FILE *f;
    SAP_VM vm;
    
    // Получаем имя виртуальной машины и аргументы
#ifdef _WIN32
    // Win32
    char argv0[2048];
    GetModuleFileNameA(0, argv0, sizeof(argv0)-1);
#else
    // Linux
    char argv0[2048];
    {
	int l=readlink("/proc/self/exe", argv0, sizeof(argv0)-1);
	if (l > 0)
	    argv0[l]=0; else
	    strcpy(argv0, argv[0]);
    }
#endif
    
    // Смотрим - может быть есть файл .sapexe с тем же именем, что и виртуальная машина
    char exename[strlen(argv0)+20];
    strcpy(exename, argv0);
    if ( (strlen(exename) > 4) && (!strcasecmp(exename+strlen(exename)-4, ".exe")) )
	exename[strlen(exename)-4]=0;
    strcat(exename, ".sapexe");
    f=fopen(exename, "rb");
    if (!f)
    {
	// Обычный запуск
	if (argc<2)
	{
	    printf("Usage: %s <program.sapexe> [parameters]\n", argv[0]);
	    return -1;
	}
	f=fopen(argv[1], "rb");
	if (!f)
	{
	    perror(argv[1]);
	    return -1;
	}
	argv++;
	argc--;
    }
    
    // Загружаем файл в память
    fread(sapexe, sizeof(sapexe), 1, f);
    fclose(f);
    
    
#ifdef DEBUG_HEAP    
    // Инитим считалку памяти
    int i;
    for (i=0; i<LIST_N; i++)
	known_obj[i]=0;
#endif
    
    // Инитим виртуальную машину
    vm=sapvm_init(argc, argv);
    if (!vm)
    {
	printf("Init error\n");
	return -1;
    }
    
    //printf("\tinit ok\n");
    
    // Запускаем
    sapvm_prepare_execution(vm);
    sap_byte ret=sapvm_execute(vm);
    
    //printf("\tret=%d fault_cause=%d\n", ret, vm->fault_cause);
    
    if (ret==SAP_FAULT)
    {
	sapvm_show_error(vm);
    }
    
    
#if defined(SAPVM_STATS)
    printf("opcodes execution statistics (total %d):\n", vm->bytecodes_executed_total);
    {
	int i;
	for (i=0; i<256; i++)
	{
	    if (vm->bytecodes_executed[i]!=0)
	    {
		printf("%s\t%d\n", op_name[i], vm->bytecodes_executed[i]);
	    }
	}
    }
#endif

    // Удаляем виртуальную машину
    sapvm_done(vm);
    
    
    //printf("mem alloc=%d free=%d\n",n_alloc, n_free);
    
    
    // Дамп неудаленных объектов
#ifdef DEBUG_HEAP
    {
	int n=0;
	
	printf("undeleted objects:\n");
	for (i=0; i<LIST_N; i++)
	{
	    if (known_obj[i])
	    {
		sap_byte *bptr=(sap_byte*)known_obj[i];
		sap_object *o=(sap_object*)(bptr+3);
		
		printf("at 0x%08x %s(%d) ID=0x%04x\n", (unsigned int)bptr, types[obj_types[i]], (((unsigned char)bptr[1])<<8) | (((unsigned char)bptr[2]) & 0xff), o->ID/*, ((((o->ID>>12)&0x07)==3) || ((o->ID>>8)==0))?sapvm_find_class_name(vm, o->ID & 0xff):""*/ );
		
		n++;
		if (n>50) break;
	    }
	}
    }
#endif
    
    return 0;
}
