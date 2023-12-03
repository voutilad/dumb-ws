use std::net::TcpListener;
use tungstenite::Message;
use tungstenite::accept;

fn main() {
    let server = TcpListener::bind("127.0.0.1:8000").unwrap();

    loop {
        let (socket, addr) = server.accept().unwrap();

        println!("got connection from: {}", addr);
        let mut websocket = accept(socket).unwrap();

        loop {
            match websocket.read().unwrap() {
                msg @ Message::Binary(_) => {
                    println!("got: {}", msg);
                    websocket.write(msg).unwrap();
                    websocket.flush().unwrap();
                },
                Message::Close(_) => {
                    println!("connection closed");
                    match websocket.close(None) {
                        Ok(_) => websocket.flush().unwrap(),
                        Err(_) => {}
                    }
                    break;
                },
                _ => { }
            }
        }
    }
}
