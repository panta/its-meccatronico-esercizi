package main

import (
	"fmt"
	"net/http"
	"os"
	"time"

	_ "github.com/go-sql-driver/mysql"
	"github.com/jmoiron/sqlx"
)

type Sample struct {
	Id   int    `db:"id"`
	Name string `db:"name"`
}

func getHandler(conn *sqlx.DB) func(http.ResponseWriter, *http.Request) {
	return func(w http.ResponseWriter, r *http.Request) {
		samples := []Sample{}
		err := conn.Select(&samples, "select * from sample")
		if err != nil {
			panic(err)
		}

		for _, sample := range samples {
			fmt.Fprintf(w, "%d - %s\n", sample.Id, sample.Name)
		}
	}
}

func main() {
	port := os.Getenv("PORT")
	if port == "" {
		port = "8080"
	}

	time.Sleep(5 * time.Second)
	dbDSN := os.Getenv("DSN")
	conn, err := sqlx.Connect("mysql", dbDSN)
	if err != nil {
		panic(err)
	}

	fmt.Printf("Listening on port %s\n", port)
	http.HandleFunc("/", getHandler(conn))
	http.ListenAndServe(fmt.Sprintf(":%s", port), nil)
}
