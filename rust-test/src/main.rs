use std::net::TcpListener;
use tungstenite::Message;
use tungstenite::server::accept;

fn main() {
    let server = TcpListener::bind("127.0.0.1:8000").unwrap();

    loop {
        let (socket, addr) = server.accept().unwrap();

        println!("got connection from: {}", addr);
        let mut websocket = accept(socket).unwrap();

        loop {
            match websocket.read_message().unwrap() {
                msg @ Message::Binary(_) => {
                    println!("got: {}", msg);
                    websocket.write_message(msg).unwrap();
                },
                Message::Close(_) => {
                    println!("connection closed");
                    match websocket.close(None) {
                        Ok(_) => websocket.write_pending().unwrap(),
                        Err(_) => {}
                    }
                    break;
                },
                _ => { }
            }
        }
    }
}
