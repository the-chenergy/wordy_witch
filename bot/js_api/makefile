./build/bot.wasm: ./build/bot.mjs
./build/bot.d.ts: ./build/bot.mjs
./build/bot.mjs: ./js_api.cc
	mkdir -p ./build
	em++ -std=c++20 -O3 --bind \
		-s ALLOW_MEMORY_GROWTH \
		-s TOTAL_MEMORY=536870912 \
		-s ENVIRONMENT='web' \
		-s MODULARIZE \
		-s ASSERTIONS \
		-o ./build/bot.mjs \
		--embind-emit-tsd bot.d.ts \
		./js_api.cc
	mkdir -p ../../web_client/src/library/bot
	cp ./build/bot.wasm \
		./build/bot.mjs \
		./build/bot.d.ts \
		../../web_client/src/library/bot

all: ./build/bot.wasm ./build/bot.mjs ./build/bot.d.ts

clean:
	rm -f ./build/bot.wasm \
		./build/bot.mjs \
		./build/bot.mjs \
		../../web_client/src/library/bot/bot.wasm \
		../../web_client/src/library/bot/bot.mjs \
		../../web_client/src/library/bot/bot.d.ts

.PHONY: all clean
