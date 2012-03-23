#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/timex.h>
#include <sys/unistd.h>
#include <sched.h>
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

void thread(int *cpu)
{
        int i;
        unsigned long long ret = 0;

        for (i = 0; i < COUNT; i++) {
                rdtscll(ret);
                
                printf("### CPU %d Count = %d TSC = %lld\n",*cpu,i,ret);
        }

}

int main(int argc, const char *argv[])
{
        int cpu_count = sysconf(_SC_NPROCESSORS_CONF);
        int i,ret;
        pthread_t td[cpu_count];
        cpu_set_t mask[cpu_count];

        for (i = 0; i < cpu_count; i++) {
                CPU_ZERO(&mask[i]);
        }

        for (i = 0; i < cpu_count; i++) {
                CPU_SET(i,&mask[i]);
        }

        for (i = 0; i < cpu_count; i++) {
                if ((ret = pthread_create(&td[i],NULL,(void *)thread,&i))){
                        printf("Create thead %d fail.Exit now. \n",i);
                        return 2;
                }

                pthread_setaffinity_np(td[i], sizeof(cpu_set_t), &mask[i]);
        }

        for (i = 0; i < cpu_count; i++) {
                pthread_join(td[i],NULL);
        }

        return 0;

}
