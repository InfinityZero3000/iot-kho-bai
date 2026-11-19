#!/bin/sh
# Chạy broker MQTT + Node-RED. Token Blynk đọc từ node-red/.env (BLYNK_TOKEN=...)
cd "$(dirname "$0")"
mkdir -p .data  # thư mục runtime bị .gitignore, clone mới chưa có
[ -f .env ] && { set -a; . ./.env; set +a; }
npx -y aedes-cli start --port 1883 > .data/broker.log 2>&1 &
trap 'kill $!' EXIT
npx -y node-red --userDir "$PWD/.data" flows.json
