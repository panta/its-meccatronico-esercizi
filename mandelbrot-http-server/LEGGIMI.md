LEGGIMI
=======

Semplice server web (HTTP) che effettua il rendering dell'insieme di Mandelbrot.

## Utilizzo

Per compilare:

```bash
$ make
```

oppure:

```bash
$ gcc -std=c99 -Wall -O2 -o mandelbrot mandelbrot.c -lm
```

avviare con:

```bash
$ ./mandelbrot
```

e quindi visitare:

    http://127.0.0.1:8080/800/600/-2/-1/1/1

oppure, per alcune "destinazioni" selezionate:

    http://127.0.0.1:8080/
