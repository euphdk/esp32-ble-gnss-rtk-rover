.PHONY: build test upload monitor

ENV ?= esp32-c3-lc29h-da-oled

build:
	pio run -e $(ENV)

test:
	sh test/host/run.sh

upload:
	pio run -e $(ENV) -t upload

monitor:
	pio device monitor -e $(ENV)
