package main

import (
	"log"
	"net/http"
	"os"
	"path/filepath"

	"git.sr.ht/~sircmpwn/getopt"
	ws "github.com/gorilla/websocket"
)

const DefaultPort = "8000"
const DefaultHost = ""

var upgrader = ws.Upgrader{}

var echo = false

func handler(w http.ResponseWriter, r *http.Request) {
	c, err := upgrader.Upgrade(w, r, nil)
	if err != nil {
		log.Fatal("Upgrade:", err)
	}
	log.Printf("got a connection from: %s", r.RemoteAddr)
	defer c.Close()

	for {
		msgtype, message, err := c.ReadMessage()

		if ws.IsUnexpectedCloseError(err, ws.CloseNoStatusReceived) {
			log.Fatal("Unexpected Close Error: ", err)
		}

		if ws.IsCloseError(err, ws.CloseNoStatusReceived) {
			// bail out of trying to read message as it's not there
			log.Printf("asked to close by: %s", r.RemoteAddr)
			break
		}

		switch msgtype {
		case ws.TextMessage:
			log.Fatal("unsupported TextMessage received!")
		case ws.BinaryMessage:
			log.Printf("got: %s", message)
			if echo {
				out := []byte("You said: ")
				out = append(out, message...)
				if c.WriteMessage(ws.BinaryMessage, out) != nil {
					log.Fatal("WriteMessage: ", err)
				}
				log.Printf("sent: %s", out)
			}
		default:
			log.Fatal("dumb message type: ", msgtype)
		}
	}
}

func main() {
	useTls := false
	port := os.Getenv("PORT")
	host := os.Getenv("HOST")
	cert := "cert.pem"
	key := "key.pem"

	opts, _, err := getopt.Getopts(os.Args, "c:ek:th:p:")
	if err != nil {
		panic(err)
	}

	for _, opt := range opts {
		switch opt.Option {
		case 'c':
			cert = opt.Value
		case 'e':
			echo = true
		case 'k':
			key = opt.Value
		case 't':
			useTls = true
		case 'h':
			host = opt.Value
		case 'p':
			port = opt.Value
		}
	}

	if port == "" {
		port = DefaultPort
	}
	if host == "" {
		host = DefaultHost
	}
	cert = filepath.Clean(cert)
	key = filepath.Clean(key)

	log.Printf("starting server on port %s", port)
	http.HandleFunc("/", handler)

	if useTls {
		err := http.ListenAndServeTLS(host+":"+port, cert, key, nil)
		if err != nil {
			log.Fatal(err)
		}
	} else {
		err := http.ListenAndServe(host+":"+port, nil)
		if err != nil {
			log.Fatal(err)
		}
	}
}
