// SPDX-FileCopyrightText: 2025 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SRC_COMMANDS_BASE_COMMAND_H_
#define SRC_COMMANDS_BASE_COMMAND_H_

#include <arpa/inet.h>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "boost/program_options/options_description.hpp"
#include "boost/program_options/parsers.hpp"
#include "boost/program_options/positional_options.hpp"
#include "boost/program_options/variables_map.hpp"
#include "google/protobuf/message.h"
#include "spdlog/spdlog.h"

#include "exceptions.h"

namespace wikiopencite::citescoop::cli {
struct GlobalOptions {
  spdlog::level::level_enum log_level;
};

enum class ExitCode : std::uint8_t {
  kCliArgsError = 3,
};

// Base type for CLI commands
class BaseCommand {
 public:
  explicit BaseCommand(std::string name, std::string description);
  virtual ~BaseCommand() = default;

  virtual int Run(std::vector<std::string> args,
                  struct GlobalOptions globals) = 0;

  void PrintHelp();

  std::string description() { return description_; }

  std::string name() { return name_; }

 protected:
  std::pair<boost::program_options::variables_map,
            boost::program_options::parsed_options>
  ParseArgs(std::vector<std::string> args);

  template <typename T>
  T EnsureArgument(std::string arg,
                   // NOLINTNEXTLINE(whitespace/indent_namespace)
                   boost::program_options::variables_map args) {
    if (!args.count(arg)) {
      throw MissingArgumentException("Missing required argument " + arg);
    }

    return args[arg].as<T>();
  }

  template <class T, typename std::enable_if<std::is_base_of<
                         google::protobuf::Message, T>::value>::type* = nullptr>
  std::pair<uint32_t, std::unique_ptr<T>> ReadMessage(
      std::shared_ptr<google::protobuf::io::CodedInputStream> input) {
    // Read the message size
    uint32_t size;

    input->ReadRaw(&size, sizeof(size));
    size = ntohl(size);

    auto message = std::make_unique<T>();

    auto limit = input->PushLimit(size);
    message->ParseFromCodedStream(input.get());
    input->PopLimit(limit);

    spdlog::trace("Read message {} from disk. Size: disk: {} memory: {}",
                  message->GetTypeName(), size, message->SpaceUsedLong());

    return std::make_pair(size, std::move(message));
  }

  std::string name_;
  std::string description_;
  boost::program_options::options_description cli_options_;
  boost::program_options::positional_options_description positional_options_;
};

}  // namespace wikiopencite::citescoop::cli

#endif  // SRC_COMMANDS_BASE_COMMAND_H_
