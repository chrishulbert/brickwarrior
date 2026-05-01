# Print the Makefile by way of help:
help:
	cat Makefile

# Compile for linux:
linux: libs
	cc \
		-DSOKOL_GLCORE \
		-lX11 -lXi -lXcursor -ldl -lpthread -lm -lGL -lasound \
		-pthread \
		-O3 \
		-I sokol \
		-I stb \
		-I dr_libs \
		-std=c23 \
		src/*.c \
		-o brickwarrior

# Compile for macOS, just the binary:
macos: libs
	cc \
		-DSOKOL_METAL \
		-x objective-c \
		-fobjc-arc \
		-framework Metal -framework MetalKit -framework Cocoa -framework QuartzCore -framework AudioToolbox \
		-O3 \
		-I sokol \
		-I stb \
		-I dr_libs \
		-std=c23 \
		src/*.c \
		-o brickwarrior

# Create a macOS .app bundle:
macos-app: macos
	rm -rf BrickWarrior.app
	mkdir -p BrickWarrior.app/Contents/MacOS
	mkdir -p BrickWarrior.app/Contents/Resources/assets
	cp -R assets/img BrickWarrior.app/Contents/Resources/assets
	cp -R assets/levels BrickWarrior.app/Contents/Resources/assets
	cp -R assets/sound BrickWarrior.app/Contents/Resources/assets
	cp brickwarrior BrickWarrior.app/Contents/MacOS
	cp assets/macos/Info.plist BrickWarrior.app/Contents
	cp assets/macos/AppIcon.icns BrickWarrior.app/Contents/Resources
	# Ad-hoc code signing, no cert needed:
	codesign --force --deep --sign - BrickWarrior.app

# Compile for windows, all I know is the graphics backend so far:
win: libs
	cl \
		-DSOKOL_D3D11 \
		/std:clatest \
		etc

# Compile the shaders:
shaders: sokol-tools
	./sokol-tools-bin/bin/osx_arm64/sokol-shdc \
		--input src/shaders/basic.glsl \
		--output src/shaders/basic.h \
		--slang metal_macos:hlsl5:glsl430:wgsl

# Group all the game libs together:
libs: sokol stb dr_libs

# Checkout Sokol if needed:
sokol: sokol/README.md
sokol/README.md:
	git clone --depth 1 https://github.com/floooh/sokol.git

# Checkout Sokol tools if needed:
sokol-tools: sokol-tools-bin/README.md
sokol-tools-bin/README.md:
	git clone --depth 1 https://github.com/floooh/sokol-tools-bin.git

# Checkout STB if needed:
stb: stb/README.md
stb/README.md:
	git clone --depth 1 https://github.com/nothings/stb.git

# Checkout dr_wav if needed:
dr_libs: dr_libs/README.md
dr_libs/README.md:
	git clone --depth 1 https://github.com/mackron/dr_libs.git

# Shortcut to compile then run:
run: macos
	./brickwarrior

