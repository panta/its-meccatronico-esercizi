#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>

#define MAX_SIZE 1000000
#define MAX_THREAD 100


typedef void *(*thread_func_t) (void *);

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, thread_func_t func, void *arg);

void vec_read(double dst[], int size)
{
	for (int i = 0; i < size; i++) {
		scanf("%lg", &dst[i]);
	}
}

void vec_print(double w[], int size)
{
	for (int i = 0; i < size; i++) {
		printf("%lg ", w[i]);
	}
}

double prod_scalare(double u[], double v[], int size)
{
	double r = 0.0;

	for (int i = 0; i < size; i++) {
		r += (u[i] * v[i]);
	}

	return r;
}

typedef struct work {
	double *u;			/* vettore u */
	double *v;			/* vettore v */
	int lo;				/* primo indice */
	int hi;				/* ultimo indice */
	double result;
} work_t;

typedef work_t *work_ptr;

void *prod_scalare_thread(void *data)
{
	struct work *work_data = (struct work *)data;

	int lo = work_data->lo;
	int hi = work_data->hi;
	double *u = work_data->u;
	double *v = work_data->v;

	double r = 0.0;

	for (int i = lo; i <= hi; i++) {
		r += (u[i] * v[i]);
	}

	work_data->result = r;
	return (void *)work_data;
}

// time_ms() returns the number of ms since epoch (1 jan 1970)
double time_ms(void)
{
	struct timeval t;
	gettimeofday(&t, NULL);
	return ((t.tv_sec * (double)1000.0) + (t.tv_usec / (double)1000.0));
}

int main(int argc, const char *argv[])
{
	pthread_t thread[MAX_THREAD];
	struct work work_data[MAX_THREAD];
	int size = 3;
	int n_threads = 4;

	if (argc > 1) {
		size = atoi(argv[1]);
	}
	if (argc > 2) {
		n_threads = atoi(argv[2]);
	}

	fprintf(stderr, "size: %d\n", size);
	fprintf(stderr, "n. threads: %d\n", n_threads);

	double t_start = time_ms();
	double *u = (double *)malloc(sizeof(double) * (size_t)size);
	if (!u) {
		fprintf(stderr, "ERROR: can't alloc memory for u\n");
		exit(EXIT_FAILURE);
	}

	double *v = (double *)malloc(sizeof(double) * (size_t)size);
	if (!v) {
		fprintf(stderr, "ERROR: can't alloc memory for v\n");
		exit(EXIT_FAILURE);
	}
	double t_end = time_ms();
	fprintf(stderr, "alloc time: %lg ms\n", (t_end - t_start));

	// printf("immettere u: ");
	t_start = time_ms();
	vec_read(u, size);

	// printf("immettere v: ");
	vec_read(v, size);
	t_end = time_ms();
	fprintf(stderr, "read time: %lg ms\n", (t_end - t_start));

	// printf("u = ");
	// vec_print(u);
	// printf("\n");

	// printf("v = ");
	// vec_print(v);
	// printf("\n");

	/* creiamo e avviamo i thread */
	t_start = time_ms();
	int n_per_thread = size / n_threads;
	for (int i = 0; i < n_threads; i++) {
		work_data[i].u = u;
		work_data[i].v = v;
		work_data[i].lo = n_per_thread * i;
		work_data[i].hi = n_per_thread * (i + 1) - 1;
		if (pthread_create(&thread[i], NULL, prod_scalare_thread, &work_data[i]) < 0) {
			fprintf(stderr, "ERROR: can't create thread %d\n", i);
			exit(EXIT_FAILURE);
		}
	}

	/* aspettiamo la terminazione e mettiamo insieme i risultati */
	double r = 0.0;
	for (int i = 0; i < n_threads; i++) {
		struct work *tdata = NULL;

		pthread_join(thread[i], (void **)&tdata);

		// tdata == &work_data[i]
		// r = r + work_data[i].result;
		r = r + tdata->result;
	}
	t_end = time_ms();

	printf("u . v = %lg\n", r);
	fprintf(stderr, "calc time: %lg ms\n", (t_end - t_start));
}
