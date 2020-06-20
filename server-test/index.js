const WebSocket = require('ws');

const wss = new WebSocket.Server({ port: 8000 });

wss.on('connection', (ws, req) => {
  console.log(`got connection from ${req.connection.remoteAddress}:${req.connection.remotePort}`);

  ws.on('message', (message) => {
    try {
      const s = message.slice(0, message.indexOf(0)).toString()
      const o = JSON.parse(s)
      console.log(`[${Date.now()}] got:\n${JSON.stringify(o)}`);
    } catch (e) {
      console.error(e)
      console.log(`not JSON, but got ${message}`)
    }
    ws.send(new Buffer.from('OK!'))
  });

});
