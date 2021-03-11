#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define MAX_SIZE 1000000

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

// time_ms() returns the number of ms since epoch (1 jan 1970)
double time_ms(void)
{
	struct timeval t;
	gettimeofday(&t, NULL);
	return ((t.tv_sec * (double)1000.0) + (t.tv_usec / (double)1000.0));
}

int main(int argc, const char *argv[])
{
	int size = 3;

	if (argc > 1) {
		size = atoi(argv[1]);
	}

	fprintf(stderr, "size: %d\n", size);

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

	t_start = time_ms();
	double p = prod_scalare(u, v, size);
	t_end = time_ms();
	printf("u . v = %lg\n", p);
	fprintf(stderr, "calc time: %lg ms\n", (t_end - t_start));
}
