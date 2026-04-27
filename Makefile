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
	# Ad-hoc code signing, no cert needed:
	codesign --force --deep --sign - BrickWarrior.app

shader: sokol-tools-bin/README.md
	./sokol-tools-bin/bin/osx_arm64/sokol-shdc \
		--input src/shaders/basic.glsl \
		--output src/shaders/basic.h \
		--slang metal_macos:hlsl5:glsl430:wgsl

sokol/sokol_app.h:
	git clone --depth 1 git@github.com:floooh/sokol.git

sokol-tools-bin/README.md:
	git clone --depth 1 git@github.com:floooh/sokol-tools-bin.git

run: macos
	./brickwarrior

