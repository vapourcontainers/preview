import { createServer } from 'http';
import express from 'express';
import morgan from 'morgan';
import { Server } from 'socket.io';
import { evaluateFile, type VSScript } from '@vscloud/native';

const PORT = process.env['PORT'] || 3000;
const ENV = process.env['NODE_ENV'] || 'development';

const app = express();
const server = createServer(app);
const io = new Server(server);

let script: VSScript | undefined;

if (ENV == 'development') {
  app.use(morgan('dev'));
} else {
  app.use(morgan('combined'));
}

io.on('connection', (socket) => {
  socket.on('open-file', (path, callback) => {
    if (script) {
      script.close();
      script = undefined;
    }

    try {
      script = evaluateFile(path);
      callback(script?.videoInfo);
    } catch (e) {
      socket.emit('error', (e as unknown as Error).message);
      callback(undefined);
    }
  });

  socket.on('request-frame', (n, callback) => {
    callback(script?.getFrameSync(n));
  });
});

server.listen(PORT, () => {
  console.log(`Server up with port: ${PORT}, env: ${ENV}`);
});
