help:
	cat Makefile

macos: sokol/sokol_app.h
	cc \
		-x objective-c \
		-fobjc-arc \
		-framework Metal -framework MetalKit -framework Cocoa -framework QuartzCore \
		-I sokol \
		src/*.c \
		-o brickwarrior

sokol/sokol_app.h:
	git clone git@github.com:floooh/sokol.git
