./build/wordyWitchBot.wasm: ./build/wordyWitchBot.mjs
./build/wordyWitchBot.d.ts: ./build/wordyWitchBot.mjs
./build/wordyWitchBot.mjs: ./js_api.cc
	mkdir -p ./build
	em++ -std=c++20 -O3 --bind \
		-s ALLOW_MEMORY_GROWTH \
		-s TOTAL_MEMORY=536870912 \
		-s ENVIRONMENT='web' \
		-s MODULARIZE \
		-o ./build/wordyWitchBot.mjs \
		--embind-emit-tsd wordyWitchBot.d.ts \
		./js_api.cc
	mkdir -p ../../web_client/src/library
	cp ./build/wordyWitchBot.wasm \
		./build/wordyWitchBot.mjs \
		./build/wordyWitchBot.d.ts \
		../../web_client/src/library

all: ./build/wordyWitchBot.wasm ./build/wordyWitchBot.mjs ./build/wordyWitchBot.d.ts

clean:
	rm -f ./build/wordyWitchBot.wasm \
		./build/wordyWitchBot.mjs \
		./build/wordyWitchBot.mjs \
		../../web_client/src/library/wordyWitchBot.wasm \
		../../web_client/src/library/wordyWitchBot.mjs \
		../../web_client/src/library/wordyWitchBot.d.ts

.PHONY: all clean
