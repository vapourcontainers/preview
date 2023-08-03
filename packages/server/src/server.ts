import { createServer } from 'http';
import express from 'express';
import { Server } from 'socket.io';
import { evaluateFile, type VSScript } from '@vapourcontainers-preview/native';

const PORT = process.env['PORT'] || 3000;
const ENV = process.env['NODE_ENV'] || 'development';

const argv = process.argv.slice(2);
const scriptToLoad = argv.find((arg) => arg.toLowerCase().endsWith('.vpy'));

const app = express();
const server = createServer(app);
const io = new Server(server);

let script: VSScript | undefined;

if (ENV == 'production') {
  app.use(express.static(new URL('../../frontend/dist', import.meta.url).pathname));
}

io.on('connection', (socket) => {
  if (scriptToLoad) {
    socket.emit('file', scriptToLoad);
  }

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
