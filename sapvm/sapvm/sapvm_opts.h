#ifndef SAPVM_OPTS_H
#define SAPVM_OPTS_H


// Требуется ли отладка
//#define SAPVM_DEBUG(...)	printf(__VA_ARGS__)
#define SAPVM_DEBUG(...)	do{}while(0)


// Функция вывода
#define SAPVM_PRINTF(...)	printf(__VA_ARGS__)


// Требуется ли кэш (для компилятора требуется лишние 4кб ОЗУ, а так - зависит от таблиц)
#define SAPVM_CACHE


// Требуется ли статистика
//#define SAPVM_STATS


#endif
