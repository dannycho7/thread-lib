#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void force_sleep(int seconds) {
	struct timespec initial_spec, remainder_spec;
	initial_spec.tv_sec = (time_t)seconds;
	initial_spec.tv_nsec = 0;

	printf("Force sleep called by %d\n", pthread_self());

	int err = -1;
	while(err == -1) {
		err = nanosleep(&initial_spec,&remainder_spec);
		printf("Returned from nanosleep %d %d\n", err, pthread_self());
		initial_spec = remainder_spec;
		memset(&remainder_spec,0,sizeof(remainder_spec));
	}
}
#define enjoy_party force_sleep(1)
#define cleanup_party force_sleep(2)
#define pass_time force_sleep(1)

int threads_done = 0;

void *inc_th(void *args) {
	printf("Thread %d entering\n", pthread_self());
	pass_time;
	threads_done++;
}

int main() {
	pthread_t threads[127];

	for (int i = 0; i < 15; i++)
		pthread_create(&threads[i], NULL, inc_th, NULL);
	printf("Main thread exiting\n");
	pthread_exit(0);

	return 0;
}