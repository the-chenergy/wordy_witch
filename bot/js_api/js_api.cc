#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <cassert>
#include <iostream>

#include "../bot.hh"
#include "../log.hh"

static wordy_witch::word_bank bank;

static wordy_witch::bot_cache bot_cache;

void load_bank(const std::vector<std::string>& words, int num_targets) {
  wordy_witch::load_bank(bank, words, num_targets);
}

EMSCRIPTEN_BINDINGS() {
  emscripten::register_vector<std::string>("StringVector");

  emscripten::function("loadBank", &load_bank);
}
