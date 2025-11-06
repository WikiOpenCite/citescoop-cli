// SPDX-FileCopyrightText: 2025 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SRC_LANGMAP_H_
#define SRC_LANGMAP_H_

#include <cstring>
#include <string>

#include "citescoop/proto/language.pb.h"

namespace wikiopencite::citescoop::cli {

wikiopencite::proto::Language WikipediaCodeToLanguage(
    // NOLINTNEXTLINE(whitespace/indent_namespace)
    const char* code,
    // NOLINTNEXTLINE(whitespace/indent_namespace)
    size_t len);
wikiopencite::proto::Language WikipediaCodeToLanguage(const std::string& code);
wikiopencite::proto::Language WikipediaCodeToLanguage(const char* code);
wikiopencite::proto::Language WikipediaDomainToLanguage(const char* domain);
wikiopencite::proto::Language WikipediaDomainToLanguage(
    // NOLINTNEXTLINE(whitespace/indent_namespace)
    const std::string& domain);

}  // namespace wikiopencite::citescoop::cli
#endif  // SRC_LANGMAP_H_
