.PHONY: all clean

all:
	make -C overlay
	make -C keyboard_module
	make -C overlay_ldr

clean:
	make -C overlay clean
	make -C keyboard_module clean
	make -C overlay_ldr clean
	rm *.bin
