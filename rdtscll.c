#include <stdio.h>
#include <time.h>
#include <sys/types.h>

#define COUNT 100000000

typedef unsigned long long u64;

#ifdef __x86_64__
#define DECLARE_ARGS(val, low, high)    unsigned low, high
#define EAX_EDX_VAL(val, low, high)     ((low) | ((u64)(high) << 32))
#define EAX_EDX_ARGS(val, low, high)    "a" (low), "d" (high)
#define EAX_EDX_RET(val, low, high)     "=a" (low), "=d" (high)
#else
#define DECLARE_ARGS(val, low, high)    unsigned long long val
#define EAX_EDX_VAL(val, low, high)     (val)
#define EAX_EDX_ARGS(val, low, high)    "A" (val)
#define EAX_EDX_RET(val, low, high)     "=A" (val)
#endif

static __always_inline unsigned long long __native_read_tsc(void)
{      
        DECLARE_ARGS(val, low, high); // unsigned low, high;

        asm volatile("rdtsc" : EAX_EDX_RET(val, low, high));
        /*
        asm volatile("rdtsc"
                        :"=a" (low), "=d" (high));
        */
       
        return EAX_EDX_VAL(val, low, high);
        // ((low) | ((u64)(high) << 32))
}


#define rdtscll(val) ((val) = __native_read_tsc()) 


int main(int argc, const char *argv[])
{
	int i;
	unsigned long long ret = 0;

	for (i = 0; i < COUNT; i++) {
		rdtscll(ret);

		printf("Count = %d TSC = %lld\n",i,ret);
	}
	return 0;
}
