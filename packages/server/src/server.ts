import express from 'express';
import morgan from 'morgan';

const PORT = process.env['PORT'] || 3000;
const ENV = process.env['NODE_ENV'] || 'development';

const app = express();

if (ENV == 'development') {
  app.use(morgan('dev'));
} else {
  app.use(morgan('combined'));
}

const server = app.listen(PORT, () => {
  console.log(`Server up with port: ${PORT}, env: ${ENV}`);
});

process.once('SIGTERM', () => {
  server.close();
  process.exit();
});
