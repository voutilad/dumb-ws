const fs = require('fs')
const https = require('https')

const WebSocket = require('ws')

const server = https.createServer({
  cert: fs.readFileSync('../cert.pem'),
  key: fs.readFileSync('../key.pem'),
})

const wss = new WebSocket.Server({ server })

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

server.listen(8443)
console.log(`listening on port 8443`)
