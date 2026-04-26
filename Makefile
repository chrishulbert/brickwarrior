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

macos-app: macos
	rm -rf BrickWarrior.app
	mkdir -p BrickWarrior.app/Contents/MacOS
	mkdir -p BrickWarrior.app/Contents/Resources
	cp brickwarrior BrickWarrior.app/Contents/MacOS
	cp assets/macos/Info.plist BrickWarrior.app/Contents
	cp assets/macos/AppIcon.icns BrickWarrior.app/Contents/Resources

sokol/sokol_app.h:
	git clone git@github.com:floooh/sokol.git
