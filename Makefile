.PHONY: build test upload monitor

ENV ?= esp32-s3

build:
	pio run -e $(ENV)

test:
	sh test/host/run.sh

upload:
	pio run -e $(ENV) -t upload

monitor:
	pio device monitor -e $(ENV)
