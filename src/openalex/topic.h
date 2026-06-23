// SPDX-FileCopyrightText: 2026 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SRC_OPENALEX_TOPIC_H_
#define SRC_OPENALEX_TOPIC_H_

#include <memory>

#include "cli.h"

namespace wikiopencite::citescoop::cli::openalex {
std::unique_ptr<Topic> NewOpenalexTopic();
}

#endif  // SRC_OPENALEX_TOPIC_H_
