.PHONY: build test upload monitor

build:
	pio run -e esp32-s3

test:
	sh test/host/run.sh

upload:
	pio run -e esp32-s3 -t upload

monitor:
	pio device monitor -e esp32-s3
