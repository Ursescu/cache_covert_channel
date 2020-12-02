#include <stdio.h>
#include <stdint.h>

inline void
clflush(volatile void *p);
inline void
test();
inline uint64_t
rdtsc();

void
clflush(volatile void *p)
{
    asm volatile ("clflush (%0)" :: "r"(p));
}

uint64_t
rdtsc()
{
    unsigned long a, d;
    asm volatile ("rdtsc" : "=a" (a), "=d" (d));
    return a | ((uint64_t)d << 32);
}

volatile int i;

uint64_t measure_acces_time(uintptr_t addr)
{
	uint64_t cycles;

	asm volatile("mov %1, %%r8\n\t"
	"lfence\n\t"
	"rdtsc\n\t"
	"mov %%eax, %%edi\n\t"
	"mov (%%r8), %%r8\n\t"
	"lfence\n\t"
	"rdtsc\n\t"
	"sub %%edi, %%eax\n\t"
	: "=a"(cycles) /*output*/
	: "r"(addr)
	: "r8", "edi");	

	return cycles;
}

void
test()
{
    printf("took %lu ticks\n", measure_acces_time((uintptr_t)&i));
}

int
main(int ac, char **av)
{
    test();
    test();
    printf("flush: ");
    clflush(&i);
    test();
    test();
    return 0;
}