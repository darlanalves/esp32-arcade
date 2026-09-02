build:
	arduino-cli compile -b esp32-bluepad32:esp32:esp32

flash: build
	arduino-cli upload -b esp32-bluepad32:esp32:esp32 -p $$(ls /dev/cu.usbserial*)

monitor: flash
	arduino-cli monitor -b esp32-bluepad32:esp32:esp32 -p $$(ls /dev/cu.usbserial*) --config 115200