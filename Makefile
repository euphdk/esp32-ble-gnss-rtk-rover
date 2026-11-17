.PHONY: build test upload monitor

ENV ?= xiao-esp32-c6-lc29h-da-oled
PORT ?=

build:
	pio run -e $(ENV)

test:
	sh test/host/run.sh

upload:
	pio run -e $(ENV) -t upload $(if $(PORT),--upload-port "$(PORT)")

monitor:
	pio device monitor -e $(ENV) $(if $(PORT),--port "$(PORT)")
