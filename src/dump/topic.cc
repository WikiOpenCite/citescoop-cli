// SPDX-FileCopyrightText: 2026 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#include "topic.h"

#include <memory>

// #include "cat.h"
#include "cli.h"
// #include "combine.h"
#include "extract.h"

// #include "meta.h"

namespace wikiopencite::citescoop::cli::dump {

std::unique_ptr<Topic> NewDumpTopic() {
  auto topic = std::make_unique<Topic>(
      "dump", "Process and inspect Wikimedia dump files");

  //topic->Register(std::shared_ptr<Command>(new CatCommand()));
  // topic->Register(std::shared_ptr<Command>(new CombineCommand()));
  topic->Register(std::shared_ptr<Command>(new ExtractCommand()));
  //topic->Register(std::shared_ptr<Command>(new MetaCommand()));
  return topic;
}
}  // namespace wikiopencite::citescoop::cli::dump
