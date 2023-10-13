#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <optional>
#include <vector>

#include "../bot.hh"
#include "../log.hh"

int main() {
  auto read_bank =
      [](wordy_witch::word_bank& out_bank, std::filesystem::path dict_path,
         wordy_witch::word_bank_guesses_inclusion guesses_inclusion) -> void {
    auto read_and_append_words =
        [](std::vector<std::string>& words,
           std::filesystem::path word_list_path) -> void {
      std::ifstream file(word_list_path);
      for (std::string word; file >> word;) {
        words.push_back(word);
      }
    };

    std::vector<std::string> words;
    read_and_append_words(words, dict_path / "targets.txt");
    int num_targets = words.size();
    if (guesses_inclusion !=
        wordy_witch::word_bank_guesses_inclusion::TARGETS_ONLY) {
      read_and_append_words(words, dict_path / "common_guesses.txt");
      if (guesses_inclusion ==
          wordy_witch::word_bank_guesses_inclusion::ALL_WORDS) {
        read_and_append_words(words, dict_path / "uncommon_guesses.txt");
      }
    }
    wordy_witch::load_bank(out_bank, words, num_targets);
  };
  static wordy_witch::word_bank bank;
  read_bank(bank, "../../bank/co_wordle_unlimited",
            wordy_witch::word_bank_guesses_inclusion::ALL_WORDS);

  std::vector<std::string> state = {
      "LEAST",
  };
  wordy_witch::guess_cost_function get_guess_cost;
  get_guess_cost = wordy_witch::get_flat_guess_cost;
  get_guess_cost = [](int num_attempts_used) -> double {
    return num_attempts_used + (num_attempts_used >= 4) * 1E6;
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

  static wordy_witch::bot_cache bot_cache = {};

  auto find_and_display_best_guess =
      [](const wordy_witch::word_bank& bank, wordy_witch::bot_cache& cache,
         int num_attempts_used, const wordy_witch::word_list& remaining_words,
         wordy_witch::guess_cost_function get_guess_cost) -> void {
    std::cout << "Candidate best guesses in this board state:" << std::endl;
    std::cout << "(Guess: a candidate best guess in this board state, after "
                 "basic pruning by entropy)"
              << std::endl;
    std::cout << "(Cost: total cost to win every remaining possible Wordle "
                 "game, under best play after guessing this word, or `inf` if "
                 "this guess results in losing at least one Wordle even under "
                 "best play)"
              << std::endl;
    std::cout << "(EC: the expected cost for a possible remaining Wordle game; "
                 "this is equal to `Cost` divided by the number of "
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
    std::cout << "(EA: the expected number of attempts needed to solve a "
                 "possible remaining Wordle game, following the best strategy)"
              << std::endl;
    std::cout << "(AD: the attempt distribution following the best strategy)"
              << std::endl;
    std::cout << "Guess\tCost\tEC\tH\tNVG\tLVG\tH2\tEA\tAD" << std::endl;

    auto display_candidate_info =
        [&bank, &cache, num_attempts_used, &remaining_words,
         &get_guess_cost](wordy_witch::candidate_info candidate) -> void {
      WORDY_WITCH_TRACE("Analyzed verdict remaining_words", candidate.guess,
                        candidate.cost);
      wordy_witch::guess_heuristic heuristic =
          wordy_witch::compute_guess_heuristic(bank, remaining_words,
                                               candidate.guess);
      std::cout << bank.words[candidate.guess] << "\t" << candidate.cost << "\t"
                << candidate.cost / remaining_words.num_targets << "\t"
                << heuristic.entropy << "\t"
                << heuristic.num_verdict_groups_with_targets << "\t"
                << heuristic.num_targets_in_largest_verdict_group << "\t"
                << heuristic.entropy +
                       wordy_witch::compute_next_attempt_entropy(
                           bank, remaining_words, candidate.guess);

      std::optional<wordy_witch::strategy> strategy =
          wordy_witch::find_best_strategy(bank, cache,
                                          wordy_witch::MAX_NUM_ATTEMPTS_ALLOWED,
                                          num_attempts_used, remaining_words,
                                          candidate.guess, get_guess_cost);
      if (strategy.has_value()) {
        std::cout << "\t"
                  << strategy.value().total_num_attempts_used * 1.0 /
                         remaining_words.num_targets;
        for (int i = 0; i < wordy_witch::MAX_NUM_ATTEMPTS_ALLOWED; i++) {
          std::cout << "\t"
                    << strategy.value().num_targets_solved_by_attempts_used[i];
        }
      }

      std::cout << std::endl;
    };
    wordy_witch::candidate_info best_guess = wordy_witch::find_best_guess(
        bank, cache, wordy_witch::MAX_NUM_ATTEMPTS_ALLOWED, num_attempts_used,
        remaining_words, display_candidate_info, get_guess_cost);
    std::cout << std::endl;

    std::cout << "Best guess in the input board state: "
              << bank.words[best_guess.guess]
              << " (GL: " << remaining_words.num_words
              << ", TL: " << remaining_words.num_targets
              << ", Cost: " << best_guess.cost
              << ", EC: " << best_guess.cost / remaining_words.num_targets
              << ")" << std::endl;
  };

  auto find_and_display_best_guess_by_verdict =
      [](const wordy_witch::word_bank& bank, wordy_witch::bot_cache& cache,
         int num_attempts_used, const wordy_witch::word_list& remaining_words,
         const std::string& prev_guess,
         wordy_witch::guess_cost_function get_guess_cost) -> void {
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
    std::cout << "(Cost: total cost to win every remaining possible Wordle "
                 "game, under best play after guessing this word, or `inf` if "
                 "this guess results in losing at least one Wordle even under "
                 "best play)"
              << std::endl;
    std::cout << "(EC: the expected cost for a possible remaining Wordle game; "
                 "this is equal to `Cost` divided by the number of "
                 "remaining possible target words)"
              << std::endl;
    std::cout << "(H: the Shannon entropy of the best guess)" << std::endl;
    std::cout << "(NVG: the number of verdict groups the best produces)"
              << std::endl;
    std::cout
        << "(LVG: the number of possible target words in the largest verdict "
           "remaining_words after guessing the best guess)"
        << std::endl;
    std::cout << "VID\tLG\tV\tNG\tGL\tTL\tCost\tEC\tH\tNVG\tLVG" << std::endl;

    int guess = wordy_witch::find_word(bank, prev_guess).value();
    auto display_best_guess_for_verdict_group =
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
                << verdict_group.num_words << "\t" << verdict_group.num_targets
                << "\t" << best_guess.cost << "\t"
                << best_guess.cost / verdict_group.num_targets << "\t"
                << heuristic.entropy << "\t"
                << heuristic.num_verdict_groups_with_targets << "\t"
                << heuristic.num_targets_in_largest_verdict_group << std::endl;
    };
    double cost = wordy_witch::evaluate_guess(
        bank, cache, wordy_witch::MAX_NUM_ATTEMPTS_ALLOWED, num_attempts_used,
        remaining_words, guess, display_best_guess_for_verdict_group,
        get_guess_cost);
    std::cout << std::endl;

    wordy_witch::guess_heuristic heuristic =
        wordy_witch::compute_guess_heuristic(bank, remaining_words, guess);
    std::cout << "Overall, best play after guessing " << prev_guess
              << " (H: " << heuristic.entropy
              << ", NVG: " << heuristic.num_verdict_groups_with_targets
              << ", LVG: " << heuristic.num_targets_in_largest_verdict_group
              << ") produces a mean cost of "
              << cost / remaining_words.num_targets
              << " per Wordle game (GL: " << remaining_words.num_words
              << ", TL: " << remaining_words.num_targets << ", Cost: " << cost
              << ")" << std::endl;
  };

  if (state.size() % 2 == 0) {
    find_and_display_best_guess(bank, bot_cache, state.size() / 2,
                                remaining_words, get_guess_cost);
  } else {
    find_and_display_best_guess_by_verdict(
        bank, bot_cache, state.size() / 2 + 1, remaining_words, state.back(),
        get_guess_cost);
  }
  std::cout << std::endl;

  auto find_and_display_best_strategy =
      [](const wordy_witch::word_bank& bank, int num_attempts_used,
         const wordy_witch::word_list& remaining_words,
         std::optional<int> prev_guess,
         wordy_witch::guess_cost_function get_guess_cost) -> void {
    wordy_witch::strategy strategy =
        wordy_witch::find_best_strategy(
            bank, bot_cache, wordy_witch::MAX_NUM_ATTEMPTS_ALLOWED,
            num_attempts_used, remaining_words, prev_guess, get_guess_cost)
            .value();

    std::cout << "Best guess after \"" << bank.words[strategy.guess]
              << "\" in every possible scenario:";
    std::function<void(const wordy_witch::word_bank&, wordy_witch::strategy,
                       int)>
        display_strategy = [&display_strategy, num_attempts_used](
                               const wordy_witch::word_bank& bank,
                               wordy_witch::strategy strategy,
                               int indent_level) -> void {
      if (indent_level > 0) {
        std::cout << bank.words[strategy.guess]
                  << "\t(GL:" << strategy.num_remaining_words
                  << ", TL: " << strategy.num_remaining_targets << ", EA: "
                  << strategy.total_num_attempts_used * 1.0 /
                         strategy.num_remaining_targets
                  << ")";
      }
      for (int verdict = wordy_witch::NUM_VERDICTS; verdict >= 0; verdict--) {
        if (strategy.follow_ups_by_verdict.count(verdict) == 0) {
          continue;
        }
        std::cout << std::endl;
        std::cout << std::string(indent_level, '\t')
                  << bank.words[strategy.guess] << " "
                  << wordy_witch::format_verdict(verdict) << " ";
        display_strategy(bank, strategy.follow_ups_by_verdict[verdict].value(),
                         indent_level + 1);
      }
    };
    display_strategy(bank, strategy, 0);
    std::cout << std::endl;
    std::cout << std::endl;

    std::cout << "Overall, the best strategy (starting with \""
              << bank.words[strategy.guess] << "\") produces a mean of "
              << strategy.total_num_attempts_used * 1.0 /
                     remaining_words.num_targets
              << " attempts per Wordle game (total attempts: "
              << strategy.total_num_attempts_used << ")" << std::endl;
    std::cout << "Attempt distribution:" << std::endl;
    for (int i = 0; i < wordy_witch::MAX_NUM_ATTEMPTS_ALLOWED; i++) {
      if (i > 0) {
        std::cout << "\t";
      }
      std::cout << strategy.num_targets_solved_by_attempts_used[i];
    }
    std::cout << std::endl;
    std::cout << "Attempt distribution percentages:" << std::endl;
    for (int i = 0; i < wordy_witch::MAX_NUM_ATTEMPTS_ALLOWED; i++) {
      if (i > 0) {
        std::cout << "\t";
      }
      std::cout << strategy.num_targets_solved_by_attempts_used[i] * 100.0 /
                       remaining_words.num_targets;
    }
    std::cout << std::endl;
  };

  std::optional<int> prev_guess;
  if (state.size() % 2 == 1) {
    prev_guess = wordy_witch::find_word(bank, state.back()).value();
  }
  find_and_display_best_strategy(bank, state.size() / 2, remaining_words,
                                 prev_guess, get_guess_cost);
}
