# Print the Makefile by way of help:
help:
	cat Makefile

# Compile for macOS, just the binary:
macos: sokol stb
	cc \
		-x objective-c \
		-fobjc-arc \
		-framework Metal -framework MetalKit -framework Cocoa -framework QuartzCore \
		-I sokol \
		-I stb \
		src/*.c \
		-o brickwarrior

# Create a macOS .app bundle:
macos-app: macos
	rm -rf BrickWarrior.app
	mkdir -p BrickWarrior.app/Contents/MacOS
	mkdir -p BrickWarrior.app/Contents/Resources
	cp brickwarrior BrickWarrior.app/Contents/MacOS
	cp assets/macos/Info.plist BrickWarrior.app/Contents
	cp assets/macos/AppIcon.icns BrickWarrior.app/Contents/Resources
	# Ad-hoc code signing, no cert needed:
	codesign --force --deep --sign - BrickWarrior.app

# Compile the shaders:
shaders: sokol-tools
	./sokol-tools-bin/bin/osx_arm64/sokol-shdc \
		--input src/shaders/basic.glsl \
		--output src/shaders/basic.h \
		--slang metal_macos:hlsl5:glsl430:wgsl

# Checkout Sokol if needed:
sokol: sokol/README.md
sokol/README.md:
	git clone --depth 1 git@github.com:floooh/sokol.git

# Checkout Sokol tools if needed:
sokol-tools: sokol-tools-bin/README.md
sokol-tools-bin/README.md:
	git clone --depth 1 git@github.com:floooh/sokol-tools-bin.git

# Checkout STB if needed:
stb: stb/README.md
stb/README.md:
	git clone --depth 1 git@github.com:nothings/stb.git

# Shortcut to compile then run:
run: macos
	./brickwarrior

