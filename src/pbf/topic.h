// SPDX-FileCopyrightText: 2026 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SRC_PBF_TOPIC_H_
#define SRC_PBF_TOPIC_H_

#include <memory>

#include "cli.h"

namespace wikiopencite::citescoop::cli::pbf {
std::unique_ptr<Topic> NewPbfTopic();
}

#endif  // SRC_OPENALEX_TOPIC_H_
