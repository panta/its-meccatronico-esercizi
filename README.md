README
======

Collection of excercise solutions for the ITS Meccatronico
Operating Systems course.

* `esercizio-3` - computing the dot product with single/multiple threads
* `esercizio-4` - drawing the Mandelbrot set with single/multiple threads
* `docker-adder` - a minimal Docker container with a service written in C
* `esempio-docker-compose` - example of a Docker Compose service with a web service (Go) and a MySQL database
* `http-server-example` - a minimal C web server
* `mandelbrot-http-server` - a C web server that draws the Mandelbrot set
* `docker-compose-mandelbrot` - a Docker Compose service written in C thay draws the Mandelbrot set distributing the work to multiple worker containers

Some examples/excercises use these great libraries:

* [jeremycw/httpserver.h](https://github.com/jeremycw/httpserver.h) - single-header C library for building event driven non-blocking HTTP servers
* [mattiasgustavsson/libs/http.h](https://github.com/mattiasgustavsson/libs/blob/main/http.h) - single-header C client library for HTTP
* [nothings/stb/stb_image.h](https://github.com/nothings/stb/blob/master/stb_image.h) - single-header C library for PNG, JPEG, ... image loading
* [nothings/stb/stb_image_write.h](https://github.com/nothings/stb/blob/master/stb_image_write.h) - single-header C library for PNG, JPEG, ... image writing
