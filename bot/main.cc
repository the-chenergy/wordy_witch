#include <iomanip>
#include <iostream>
#include <numeric>
#include <optional>
#include <vector>

#include "bot.hh"
#include "log.hh"

int main() {
  static wordy_witch::word_bank bank;
  wordy_witch::load_bank(bank, "../bank/co_wordle",
                         wordy_witch::word_bank_guesses_inclusion::ALL_WORDS);
  std::vector<std::string> state = {
      "LEAST",
  };

  static wordy_witch::word_list remaining_words;
  remaining_words.num_words = bank.num_words;
  remaining_words.num_targets = bank.num_targets;
  std::iota(remaining_words.words, std::end(remaining_words.words), 0);

  auto display_initial_message_and_parse_state =
      [](wordy_witch::word_list& remaining_words,
         const wordy_witch::word_bank& bank,
         const std::vector<std::string>& state) -> void {
    std::cout << std::setprecision(4);
    std::cout << "WordyWitch analysis" << std::endl;
    std::cout << std::endl;
    std::cout << "Board state to analyze:" << std::endl;
    for (int i = 1; i < state.size(); i += 2) {
      int guess = wordy_witch::find_word(bank, state[i - 1]).value();
      std::cout << state[i - 1] << std::endl;
      static wordy_witch::verdict_groups groups;
      wordy_witch::group_remaining_words(groups, bank, remaining_words, guess);
      auto parse_verdict = [](const std::string& tiles) -> std::optional<int> {
        for (int verdict = 0; verdict < wordy_witch::NUM_VERDICTS; verdict++) {
          if (wordy_witch::format_verdict(verdict) == tiles) {
            return verdict;
          }
        }
        return std::nullopt;
      };
      int verdict = parse_verdict(state[i]).value();
      remaining_words = groups[verdict];
      std::cout << state[i] << std::endl;
    }
    if (state.size() % 2 == 1) {
      std::cout << state.back() << std::endl;
    }
    std::cout << std::endl;
  };
  display_initial_message_and_parse_state(remaining_words, bank, state);

  static wordy_witch::bot_cache bot_cache;
  wordy_witch::reset_cache(bot_cache);

  auto find_and_display_best_guess =
      [](const wordy_witch::word_bank& bank, int num_attempts_used,
         const wordy_witch::word_list& remaining_words) -> void {
    std::cout << "Candidate best guesses in this board state:" << std::endl;
    std::cout << "(Guess: a candidate best guess in this board state, after "
                 "basic pruning by entropy)"
              << std::endl;
    std::cout << "(Cost: total number of attempts to win every remaining "
                 "possible Wordle game, under best play after guessing this "
                 "word, or 0 if this guess resulting losing a Wordle even "
                 "under best play)"
              << std::endl;
    std::cout << "(EA: expected attempts; `Cost` divided by the number of "
                 "remaining possible target words)"
              << std::endl;
    std::cout << "(H: the Shannon entropy of this guess)" << std::endl;
    std::cout << "(NVG: the number of verdict groups this guess produces)"
              << std::endl;
    std::cout
        << "(LVG: the number of possible target words in the largest verdict "
           "remaining_words after guessing this word)"
        << std::endl;
    std::cout << "(H2: the expected entropy produced by guessing this word and "
                 "the best next guess)"
              << std::endl;
    std::cout << "Guess\tCost\tEA\tH\tNVG\tLVG\tH2" << std::endl;

    wordy_witch::candidate_info best_guess = wordy_witch::find_best_guess(
        bank, bot_cache, wordy_witch::MAX_NUM_ATTEMPTS_ALLOWED,
        num_attempts_used, remaining_words,
        [&bank,
         &remaining_words](wordy_witch::candidate_info candidate) -> void {
          WORDY_WITCH_TRACE("Analyzed verdict remaining_words", candidate.guess,
                            candidate.cost);
          wordy_witch::guess_heuristic heuristic =
              wordy_witch::compute_guess_heuristic(bank, remaining_words,
                                                   candidate.guess);
          std::cout << bank.words[candidate.guess] << "\t" << candidate.cost
                    << "\t" << candidate.cost / remaining_words.num_targets
                    << "\t" << heuristic.entropy << "\t"
                    << heuristic.num_verdict_groups_with_targets << "\t"
                    << heuristic.num_targets_in_largest_verdict_group << "\t"
                    << heuristic.entropy +
                           wordy_witch::compute_next_attempt_entropy(
                               bank, remaining_words, candidate.guess)
                    << std::endl;
        });
    std::cout << std::endl;

    std::cout << "Best guess in the input board state: "
              << bank.words[best_guess.guess]
              << " (GL: " << remaining_words.num_words
              << ", TL: " << remaining_words.num_targets
              << ", Cost: " << best_guess.cost
              << ", EA: " << best_guess.cost / remaining_words.num_targets
              << ")" << std::endl;
  };

  auto find_and_display_best_guess_by_verdict =
      [](const wordy_witch::word_bank& bank, int num_attempts_used,
         const wordy_witch::word_list& remaining_words,
         const std::string& prev_guess) -> void {
    std::cout << "Best guesses in this board state for each possible verdict:"
              << std::endl;
    std::cout << "(VID: a base-3 encoded number of this verdict)" << std::endl;
    std::cout << "(LG: your last guess)" << std::endl;
    std::cout << "(V: the verdict tiles: - black, ^ yellow, # green)"
              << std::endl;
    std::cout << "(NG: the best next guess given this verdict)" << std::endl;
    std::cout << "(GL: the number of guessable words left in Hard Mode after "
                 "this verdict shows)"
              << std::endl;
    std::cout << "(TL: the number of possible target words left after this "
                 "verdict shows)"
              << std::endl;
    std::cout << "(Cost: total number of attempts to win every remaining "
                 "possible Wordle game, under best play after guessing this "
                 "word, or 0 if this guess resulting losing a Wordle even "
                 "under best play)"
              << std::endl;
    std::cout << "(EA: expected attempts; `Cost` divided by the number of "
                 "remaining possible target words after this verdict)"
              << std::endl;
    std::cout << "(H: the Shannon entropy of the best guess)" << std::endl;
    std::cout << "(NVG: the number of verdict groups the best produces)"
              << std::endl;
    std::cout
        << "(LVG: the number of possible target words in the largest verdict "
           "remaining_words after guessing the best guess)"
        << std::endl;
    std::cout << "VID\tLG\tV\tNG\tGL\tTL\tCost\tEA\tH\tNVG\tLVG" << std::endl;

    int guess = wordy_witch::find_word(bank, prev_guess).value();
    double cost = wordy_witch::evaluate_guess(
        bank, bot_cache, wordy_witch::MAX_NUM_ATTEMPTS_ALLOWED,
        num_attempts_used, remaining_words, guess,
        [&bank, &prev_guess](int verdict,
                             const wordy_witch::word_list& verdict_group,
                             wordy_witch::candidate_info best_guess) -> void {
          WORDY_WITCH_TRACE("Analyzed candidate", verdict, best_guess.guess,
                            best_guess.cost);
          wordy_witch::guess_heuristic heuristic =
              wordy_witch::compute_guess_heuristic(bank, verdict_group,
                                                   best_guess.guess);
          std::cout << verdict << "\t" << prev_guess << "\t"
                    << wordy_witch::format_verdict(verdict) << "\t"
                    << bank.words[best_guess.guess] << "\t"
                    << verdict_group.num_words << "\t"
                    << verdict_group.num_targets << "\t" << best_guess.cost
                    << "\t" << best_guess.cost / verdict_group.num_targets
                    << "\t" << heuristic.entropy << "\t"
                    << heuristic.num_verdict_groups_with_targets << "\t"
                    << heuristic.num_targets_in_largest_verdict_group
                    << std::endl;
        });
    std::cout << std::endl;

    wordy_witch::guess_heuristic heuristic =
        wordy_witch::compute_guess_heuristic(bank, remaining_words, guess);
    std::cout << "Overall, best play after guessing " << prev_guess
              << " (H: " << heuristic.entropy
              << ", NVG: " << heuristic.num_verdict_groups_with_targets
              << ", LVG: " << heuristic.num_targets_in_largest_verdict_group
              << ") produces a mean of " << cost / remaining_words.num_targets
              << " attempts per Wordle game (GL: " << remaining_words.num_words
              << ", TL: " << remaining_words.num_targets << ", Cost: " << cost
              << ")" << std::endl;
  };

  if (state.size() % 2 == 0) {
    find_and_display_best_guess(bank, state.size() / 2, remaining_words);
  } else {
    find_and_display_best_guess_by_verdict(bank, state.size() / 2 + 1,
                                           remaining_words, state.back());
  }
}
