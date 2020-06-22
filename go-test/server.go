package main

import (
	"log"
	"net/http"

	"github.com/gorilla/websocket"
)

var upgrader = websocket.Upgrader{}

func handler(w http.ResponseWriter, r *http.Request) {
	c, err := upgrader.Upgrade(w, r, nil)
	if err != nil {
		log.Fatal("Upgrade:", err)
	}
	log.Println("got connection from ", r.RemoteAddr)
	defer c.Close()
	for {
		mt, message, err := c.ReadMessage()
		if err != nil {
			log.Fatal("ReadMesage:", err)
		}

		log.Printf("got: %s", message)
		out := []byte("got: ")
		out = append(out, message...)
		err = c.WriteMessage(mt, out)
		if err != nil {
			log.Fatal("WriteMessage:", err)
		}
		log.Println("Wrote response")
	}
}

func main() {
	log.Print("String server on port 8000...")
	http.HandleFunc("/", handler)
	http.ListenAndServe("localhost:8000", nil)
}
