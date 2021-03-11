#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

double rand_range(double min, double max)
{
	double v = (double)rand();

	return ((v / (double)RAND_MAX) * (max - min)) + min;
}

int main(int argc, const char *argv[])
{
	int size = 3;

	if (argc > 1) {
		size = atoi(argv[1]);
	}

	srand((unsigned int)time(NULL));
	rand();

	for (int i = 0; i < size; i++) {
		double el = (double)rand_range(-3.0, 3.0);
		printf("%lg ", el);
	}
	printf("\n");
}
