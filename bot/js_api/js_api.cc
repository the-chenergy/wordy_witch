#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <cassert>
#include <iostream>

#include "../bot.hh"
#include "../log.hh"

static wordy_witch::word_bank bank;

static wordy_witch::bot_cache bot_cache;

/*
  `loadBank(words: string[], numTargets: number): void`
*/
void load_bank(const emscripten::val& words, int num_targets) {
  assert(words.isArray());
  int num_words = words["length"].as<int>();
  std::vector<std::string> words_vector(num_words);
  for (int i = 0; i < num_words; i++) {
    words_vector[i] = words[i].as<std::string>();
  }
  wordy_witch::load_bank(bank, words_vector, num_targets);
}

EMSCRIPTEN_BINDINGS() {
  emscripten::function("loadBank", &load_bank);  //
}
