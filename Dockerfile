# syntax=docker/dockerfile:1.4

# stage: native

FROM ghcr.io/vapourcontainers/plugin-build-base:latest AS native

RUN <<EOF
set -eux
curl -fsSL https://deb.nodesource.com/setup_20.x | bash -
apt-get install -y nodejs
EOF

WORKDIR /app

COPY package.json package-lock.json ./
COPY packages/native/package.json ./packages/native/

RUN npm ci

COPY packages/native ./packages/native

RUN <<EOF
set -eux
npm -w @vapourcontainers-preview/native run build
mv ./packages/native/build/Release/vsrender.node ./packages/native/build/
EOF

# stage: frontend

FROM node:20-bookworm AS frontend

WORKDIR /app

COPY package.json package-lock.json ./
COPY packages/native/package.json ./packages/native/
COPY packages/frontend/package.json ./packages/frontend/

RUN npm ci

COPY --link --from=native /app/packages/native/lib ./packages/native/lib

COPY packages/frontend ./packages/frontend
RUN npm -w @vapourcontainers-preview/frontend run build

# stage: server

FROM node:20-bookworm AS server

WORKDIR /app

COPY package.json package-lock.json ./
COPY packages/native/package.json ./packages/native/
COPY packages/server/package.json ./packages/server/

RUN npm ci

COPY --link --from=native /app/packages/native/lib ./packages/native/lib

COPY packages/server ./packages/server
RUN npm -w @vapourcontainers-preview/server run build

# stage: runtime

FROM node:20-bookworm-slim

RUN <<EOF
set -eux
echo "deb http://www.deb-multimedia.org bookworm main" >> /etc/apt/sources.list
apt-get update -oAcquire::AllowInsecureRepositories=true
apt-get install -y --allow-unauthenticated --no-install-recommends deb-multimedia-keyring vapoursynth
rm -rf /var/lib/apt/lists/*
EOF

WORKDIR /app

COPY package.json package-lock.json ./
COPY packages/native/package.json ./packages/native/
COPY packages/server/package.json ./packages/server/

RUN npm ci --production

COPY --link --from=native /app/packages/native/build/*.node ./packages/native/build/
COPY --link --from=native /app/packages/native/lib ./packages/native/lib

COPY --link --from=frontend /app/packages/frontend/dist ./packages/frontend/dist

COPY --link --from=server /app/packages/server/lib ./packages/server/lib

ENV NODE_ENV=production \
    PORT=80

CMD node /app/packages/server
