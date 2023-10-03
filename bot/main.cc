#include <cstdlib>
#include <iostream>
#include <vector>

#include "bot.hh"
#include "log.hh"

int main() {
  static wordy_witch::Bank bank;
  wordy_witch::load_bank(bank, "../bank/co_wordle",
                         wordy_witch::BankGuessesInclusion::ALL_WORDS);

  static wordy_witch::Group initial_group = {};
  std::vector<int> words;
  // words = {
  //     wordy_witch::find_word(bank, "baste").value(),
  //     wordy_witch::find_word(bank, "caste").value(),
  //     wordy_witch::find_word(bank, "haste").value(),
  //     wordy_witch::find_word(bank, "paste").value(),
  //     wordy_witch::find_word(bank, "taste").value(),
  //     wordy_witch::find_word(bank, "waste").value(),

  //     wordy_witch::find_word(bank, "tapes").value(),
  // };
  for (int i = 0; i < bank.num_words; i++) {
    if (bank.words[i][0] != 'n') {
      // continue;
    }
    words.push_back(i);
  }
  for (int i = 0; i < words.size(); i++) {
    // std::cerr << bank.words[words[i]] << " ";
    initial_group.words[i] = words[i];
  }
  std::cerr << std::endl;
  initial_group.num_words = words.size();
  initial_group.num_targets = 0;
  while (initial_group.num_targets < words.size() &&
         words[initial_group.num_targets] < bank.num_targets) {
    initial_group.num_targets++;
  }

  WORDY_WITCH_TRACE(initial_group.num_words, initial_group.num_targets,
                    bank.words[initial_group.words[0]]);

  static wordy_witch::Grouping grouping;
  int opener = wordy_witch::find_word(bank, "least").value();
  wordy_witch::group_guesses(grouping, bank, initial_group, opener);
  for (int v = 0; v < wordy_witch::NUM_VERDICTS; v++) {
    wordy_witch::Group& g = grouping.groups[v];
    if (g.num_targets == 0) {
      continue;
    }
    WORDY_WITCH_TRACE(v, wordy_witch::format_verdict(v), g.num_words,
                      g.num_targets);
    // for (int i = 0; i < g.num_words; i++) {
    //   std::cerr << bank.words[g.words[i]] << " ";
    // }
    // std::cerr << std::endl;
  }

  // wordy_witch::find_best_guess(bank, 4, grouping.groups[216], 1);
  // std::exit(1);

  int cost = wordy_witch::evaluate_guess(
      bank, 5, grouping,
      [&opener](int verdict, const wordy_witch::Group& group,
                wordy_witch::BestGuessInfo best_guess) -> void {
        WORDY_WITCH_TRACE(wordy_witch::format_verdict(verdict), group.num_words,
                          group.num_targets, bank.words[best_guess.guess],
                          best_guess.cost,
                          best_guess.cost * 1.0 / group.num_targets);
        std::cout << bank.words[opener] << "\t"
                  << wordy_witch::format_verdict(verdict) << "\t"
                  << bank.words[best_guess.guess] << "\t" << group.num_words
                  << "\t" << group.num_targets << "\t"
                  << best_guess.cost * 1.0 / group.num_targets;
        std::cout << std::endl;
      });
  WORDY_WITCH_TRACE(cost, cost * 1.0 / initial_group.num_targets);
  std::cout << cost * 1.0 / initial_group.num_targets << std::endl;
}
