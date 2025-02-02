// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "REDACTEDsubmissions/training/v0_7/models/prod/minigo/cc/eval.h"

// Game options flags.
ABSL_FLAG(bool, resign_enabled, true, "Whether resign is enabled.");
ABSL_FLAG(double, resign_threshold, -0.999, "Resign threshold.");
ABSL_FLAG(uint64, seed, 0,
          "Random seed. Use default value of 0 to use a time-based seed.");

// Tree search flags.
ABSL_FLAG(int32, virtual_losses, 8,
          "Number of virtual losses when running tree search.");
ABSL_FLAG(double, value_init_penalty, 2.0,
          "New children value initialization penalty.\n"
          "Child value = parent's value - penalty * color, clamped to"
          " [-1, 1].  Penalty should be in [0.0, 2.0].\n"
          "0 is init-to-parent, 2.0 is init-to-loss [default].\n"
          "This behaves similiarly to Leela's FPU \"First Play Urgency\".");

// Inference flags.
ABSL_FLAG(std::string, eval_model, "",
          "Path to a minigo model to evaluate against a target.");
ABSL_FLAG(std::string, eval_device, "",
          "Optional ID of the device to the run inference on for the eval "
          "model. For TPUs, pass the gRPC address.");
ABSL_FLAG(int32, num_eval_readouts, 100,
          "Number of readouts to make during tree search for the eval "
          "model.)");

ABSL_FLAG(std::string, target_model, "",
          "Path to a target minigo model that eval_model is evaluated "
          "against.");
ABSL_FLAG(std::string, target_device, "",
          "Optional ID of the device to the run inference on for the "
          "target model. For TPUs, pass the gRPC address.");
ABSL_FLAG(int32, num_target_readouts, 100,
          "Number of readouts to make during tree search for the eval "
          "model.)");

ABSL_FLAG(int32, parallel_games, 32, "Number of games to play in parallel.");

// Output flags.
ABSL_FLAG(std::string, output_bigtable, "",
          "Output Bigtable specification, of the form: "
          "project,instance,table. "
          "If empty, no examples are written to Bigtable.");
ABSL_FLAG(std::string, sgf_dir, "",
          "SGF directory for selfplay and puzzles. If empty in selfplay "
          "mode, no SGF is written.");
ABSL_FLAG(std::string, bigtable_tag, "", "Used in Bigtable metadata.");
ABSL_FLAG(bool, verbose, true, "Enable verbose logging.");

namespace minigo {
Evaluator::Evaluator() {
  // Create a batcher for the eval model.
  batchers_.push_back(absl::make_unique<BatchingModelFactory>(
      absl::GetFlag(FLAGS_eval_device), 2));

  // If the target model requires a different device, create one & a second
  // batcher too.
  if (absl::GetFlag(FLAGS_target_device) != absl::GetFlag(FLAGS_eval_device)) {
    batchers_.push_back(absl::make_unique<BatchingModelFactory>(
        absl::GetFlag(FLAGS_target_device), 2));
  }
}

void Evaluator::Reset() { threads_ = std::vector<std::thread>(); }
std::vector<std::pair<std::string, WinStats>> Evaluator::Run() {
  auto start_time = absl::Now();

  game_options_.resign_enabled = absl::GetFlag(FLAGS_resign_enabled);
  game_options_.resign_threshold =
      -std::abs(absl::GetFlag(FLAGS_resign_threshold));

  MctsPlayer::Options player_options;
  player_options.virtual_losses = absl::GetFlag(FLAGS_virtual_losses);
  player_options.inject_noise = false;
  player_options.random_seed = absl::GetFlag(FLAGS_seed);
  player_options.tree.value_init_penalty =
      absl::GetFlag(FLAGS_value_init_penalty);
  player_options.tree.soft_pick_enabled = false;

  player_options.num_readouts = absl::GetFlag(FLAGS_num_eval_readouts);
  EvaluatedModel eval_model(batchers_.front().get(),
                            absl::GetFlag(FLAGS_eval_model), player_options);

  player_options.num_readouts = absl::GetFlag(FLAGS_num_target_readouts);
  EvaluatedModel target_model(batchers_.back().get(),
                              absl::GetFlag(FLAGS_target_model),
                              player_options);

  int num_games = absl::GetFlag(FLAGS_parallel_games);
  for (int thread_id = 0; thread_id < num_games; ++thread_id) {
    bool swap_models = (thread_id & 1) != 0;
    threads_.emplace_back(std::bind(&Evaluator::ThreadRun, this, thread_id,
                                    swap_models ? &target_model : &eval_model,
                                    swap_models ? &eval_model : &target_model));
  }
  for (auto& t : threads_) {
    t.join();
  }

  MG_LOG(INFO) << "Evaluated " << num_games << " games, total time "
               << (absl::Now() - start_time);

  std::vector<std::pair<std::string, WinStats>> win_stats_result(
      {{eval_model.name(), eval_model.GetWinStats()},
       {target_model.name(), target_model.GetWinStats()}});
  MG_LOG(INFO) << FormatWinStatsTable(win_stats_result);
  return win_stats_result;
}

void Evaluator::ThreadRun(int thread_id, EvaluatedModel* black_model,
                          EvaluatedModel* white_model) {
  // Only print the board using ANSI colors if stderr is sent to the
  // terminal.
  const bool use_ansi_colors = FdSupportsAnsiColors(fileno(stderr));

  // The player and other_player reference this pointer.
  std::unique_ptr<Model> model;

  std::vector<std::string> bigtable_spec =
      absl::StrSplit(absl::GetFlag(FLAGS_output_bigtable), ',');
  bool use_bigtable = bigtable_spec.size() == 3;
  if (!absl::GetFlag(FLAGS_output_bigtable).empty() && !use_bigtable) {
    MG_LOG(FATAL)
        << "Bigtable output must be of the form: project,instance,table";
    return;
  }

  Game game(black_model->name(), white_model->name(), game_options_);

  const bool verbose = absl::GetFlag(FLAGS_verbose) && (thread_id == 0);
  auto black = absl::make_unique<MctsPlayer>(
      black_model->NewModel(), nullptr, &game, black_model->player_options());
  auto white = absl::make_unique<MctsPlayer>(
      white_model->NewModel(), nullptr, &game, white_model->player_options());

  BatchingModelFactory::StartGame(black->model(), white->model());
  auto* curr_player = black.get();
  auto* next_player = white.get();
  while (!game.game_over()) {
    if (curr_player->root()->position.n() >= kMinPassAliveMoves &&
        curr_player->root()->position.CalculateWholeBoardPassAlive()) {
      // Play pass moves to end the game.
      while (!game.game_over()) {
        MG_CHECK(curr_player->PlayMove(Coord::kPass));
        next_player->PlayOpponentsMove(Coord::kPass);
        std::swap(curr_player, next_player);
      }
      break;
    }

    auto move = curr_player->SuggestMove(curr_player->options().num_readouts);
    if (verbose) {
      std::cerr << curr_player->tree().Describe() << "\n";
    }
    MG_CHECK(curr_player->PlayMove(move));
    if (move != Coord::kResign) {
      next_player->PlayOpponentsMove(move);
    }
    if (verbose) {
      MG_LOG(INFO) << absl::StreamFormat(
          "%d: %s by %s\nQ: %0.4f", curr_player->root()->position.n(),
          move.ToGtp(), curr_player->name(), curr_player->root()->Q());
      MG_LOG(INFO) << curr_player->root()->position.ToPrettyString(
          use_ansi_colors);
    }
    std::swap(curr_player, next_player);
  }
  BatchingModelFactory::EndGame(black->model(), white->model());

  if (game.result() > 0) {
    black_model->UpdateWinStats(game);
  } else {
    white_model->UpdateWinStats(game);
  }

  if (verbose) {
    MG_LOG(INFO) << game.result_string();
    MG_LOG(INFO) << "Black was: " << game.black_name();
  }

  // Write SGF.
  std::string output_name = "NO_SGF_SAVED";
  if (!absl::GetFlag(FLAGS_sgf_dir).empty()) {
    output_name = absl::StrCat(GetOutputName(game_id_++), "-", black->name(),
                               "-", white->name());
    game.AddComment(
        absl::StrCat("B inferences: ", black->GetModelsUsedForInference()));
    game.AddComment(
        absl::StrCat("W inferences: ", white->GetModelsUsedForInference()));
    WriteSgf(absl::GetFlag(FLAGS_sgf_dir), output_name, game, true);
  }

  if (use_bigtable) {
    const auto& gcp_project_name = bigtable_spec[0];
    const auto& instance_name = bigtable_spec[1];
    const auto& table_name = bigtable_spec[2];
    tf_utils::WriteEvalRecord(gcp_project_name, instance_name, table_name, game,
                              output_name, absl::GetFlag(FLAGS_bigtable_tag));
  }

  MG_LOG(INFO) << "Thread " << thread_id << " stopping";
}

}  // namespace minigo
