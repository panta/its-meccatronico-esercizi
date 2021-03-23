LEGGIMI
=======

Esempio di come "calcolare" un'immagine dell'insieme di Mandelbrot
distribuendo il calcolo tra più nodi utilizzando Docker e
Docker Compose.

## Organizzazione

Nel file `docker-compose.yml` è definito un container "regista", `director`,
ed un certo numero di container "lavoratori", `workerX`.
Il container regista espone un web server alla porta 9000, dove è possibile
visitare degli url della forma:

    `/WIDTH/HEIGHT/RE0/IM0/RE1/IM1`

che causeranno il calcolo di un'immagine di Mandelbrot della
dimensione WIDTH x HEIGHT nella regione del piano complesso (RE0,IM0)-(RE1,IM1).
Il calcolo effettivo dell'immagine sarà demandato dal director ai vari worker.
Ad ognuno dei worker sarà assegnato un "sotto-rettangolo" dell'immagine
richiesta.
Il director effettua le richieste ai worker per le sotto-regioni sempre
utilizzando il protocollo HTTP (utilizzando url con il medesimo formato
indicato sopra).

## Utilizzo

Procedere al build e avvio dei container:

```bash
$ docker-compose build
$ docker-compose run
```

quindi visitare ad esempio l'url:

    http://127.0.0.1:9000/3000/2000/-2/-1/1/1

(nel caso si utilizzi Docker su linux, altrimenti utilizzare l'IP della
docker machine).
