#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    double sum = 0.0;

    for (int i = 1; i < argc; i++) {
        double val = strtod(argv[i], NULL);
        sum += val;
    }

    printf("%g\n", sum);
}
