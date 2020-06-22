const WebSocket = require('ws')
const wss = new WebSocket.Server({ port: 8000 })

wss.on('connection', (ws, req) => {
  console.log(`got connection from ${req.connection.remoteAddress}:${req.connection.remotePort}`)

  ws.on('message', (message) => {
    let m = message.toString()
    try {
      const o = JSON.parse(m)
      console.log(`got json: ${JSON.stringify(o)}`);
    } catch (e) {
      console.error(e)
      console.log(`got: ${m}`)
    }
    ws.send(new Buffer.from('You said: ' + m))
  })
})

console.log(`listening on port 8000`)
