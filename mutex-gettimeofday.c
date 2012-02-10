#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/timex.h>
#include <sys/unistd.h>
#include <sched.h>
#include <sys/types.h>

struct timeval t1; 
pthread_mutex_t global_mutex;

#define tv_lt(s,t) (s.tv_sec < t.tv_sec || (s.tv_sec == t.tv_sec && s.tv_usec < t.tv_usec))

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


void print_error(struct timeval tv_start, struct timeval tv_end)
{      
        long long time;
        struct timeval tv_diff;

        printf("start time = %10ld.%07ld \n", tv_start.tv_sec,
               tv_start.tv_usec);
        printf("end time = %10ld.%07ld \n", tv_end.tv_sec, tv_end.tv_usec);

        if(tv_start.tv_usec > tv_end.tv_usec){
                        tv_end.tv_sec--;
                        tv_end.tv_usec +=1000000L;
        }
       
        tv_diff.tv_sec = tv_end.tv_sec - tv_start.tv_sec;
        tv_diff.tv_usec = tv_end.tv_usec - tv_start.tv_usec;
        time = (long long)tv_diff.tv_sec * 1000000000L +
               (long long)tv_diff.tv_usec * 1000L;
        printf("Failed: time went backwards %lld nsec (%ld.%06ld )\n", time,
               tv_diff.tv_sec, tv_diff.tv_usec);
}

void thread(void)
{
	int i;
	struct timeval next;
	struct timeval prev;

	for (i = 0; i < 100000; i++) {
		// read 
                if (pthread_mutex_lock(&global_mutex) == 0) {
			prev = t1;
                }
		pthread_mutex_unlock(&global_mutex);

		gettimeofday(&next,NULL);

		if (tv_lt(next,prev)) {
			print_error(prev,next);
		}
   
		// write
                if (pthread_mutex_lock(&global_mutex) == 0) {
			t1 = next;
                }
		pthread_mutex_unlock(&global_mutex);
	}
}

int main(int argc, const char *argv[])
{
	int i,ret;
	int cpu_count = sysconf(_SC_NPROCESSORS_CONF);
	pthread_t td[cpu_count];
	cpu_set_t mask[cpu_count];

        for (i = 0; i < cpu_count; i++) {
        	CPU_ZERO(&mask[i]);
        }

	for (i = 0; i < cpu_count; i++) {
		CPU_SET(i,&mask[i]);
	}

	pthread_mutex_init(&global_mutex,NULL);
	
        gettimeofday(&t1,NULL);

	for (i = 0; i < cpu_count; i++) {
		if ((ret = pthread_create(&td[i],NULL,(void *)thread,NULL))){
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
