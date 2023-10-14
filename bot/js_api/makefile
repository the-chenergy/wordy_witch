./build/wordyWitchBot.wasm: ./build/wordyWitchBot.mjs

./build/wordyWitchBot.mjs: ./js_api.cc
	mkdir -p ./build
	em++ -std=c++20 -O3 --bind \
		-s TOTAL_MEMORY=536870912 \
		-s ENVIRONMENT='web' \
		-s EXPORT_NAME='createModule' \
		-o ./build/wordyWitchBot.mjs \
		./js_api.cc
	mv ./build/wordyWitchBot.wasm ./build/wordyWitchBot.mjs ../../web_client/src/library

all: ./build/wordyWitchBot.wasm

clean:
	rm ../../web_client/src/library/wordyWitchBot.wasm ../../web_client/src/library/wordyWitchBot.mjs

.PHONY: all clean
