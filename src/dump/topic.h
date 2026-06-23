// SPDX-FileCopyrightText: 2026 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SRC_DUMP_TOPIC_H_
#define SRC_DUMP_TOPIC_H_

#include <memory>

#include "cli.h"

namespace wikiopencite::citescoop::cli::dump {
std::unique_ptr<Topic> NewDumpTopic();
}

#endif  // SRC_DUMP_TOPIC_H_
