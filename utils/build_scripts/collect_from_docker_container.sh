set -ex && mkdir -p ../../build/release/bin
set -ex && docker create --name loki-daemon-container loki-daemon-image
set -ex && docker cp loki-daemon-container:/usr/local/bin/ ../../build/
set -ex && docker rm loki-daemon-container
