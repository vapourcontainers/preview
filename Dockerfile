# syntax=docker/dockerfile:1.4

# stage: native

FROM ghcr.io/vapourcontainers/plugin-build-base:latest AS native

RUN <<EOF
curl -fsSL https://deb.nodesource.com/setup_20.x | bash -
apt-get install -y nodejs
EOF

WORKDIR /app

COPY package.json package-lock.json ./
COPY packages/native/package.json ./packages/native/

RUN npm ci

COPY packages/native ./packages/native
RUN <<EOF
npm -w @vscloud/native run build
mv ./packages/native/build/Release/vscloud_native.node ./packages/native/build/
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
RUN npm -w @vscloud/frontend run build

# stage: server

FROM node:20-bookworm AS server

WORKDIR /app

COPY package.json package-lock.json ./
COPY packages/native/package.json ./packages/native/
COPY packages/server/package.json ./packages/server/

RUN npm ci

COPY --link --from=native /app/packages/native/lib ./packages/native/lib

COPY packages/server ./packages/server
RUN npm -w @vscloud/server run build

# stage: runtime

FROM node:20-bookworm

RUN <<EOF
echo "deb http://www.deb-multimedia.org bookworm main" >> /etc/apt/sources.list
apt-get update -oAcquire::AllowInsecureRepositories=true
apt-get install -y --allow-unauthenticated --no-install-recommends deb-multimedia-keyring vapoursynth
apt-get autoremove -y --purge
apt-get clean
rm -rf /var/lib/apt/lists/*
mkdir -p $HOME/.config/vapoursynth
echo "UserPluginDir=/usr/local/lib/vapoursynth" > $HOME/.config/vapoursynth/vapoursynth.conf
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

CMD ["node", "/app/packages/server"]
