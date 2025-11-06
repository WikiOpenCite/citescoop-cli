// SPDX-FileCopyrightText: 2025 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#include "langmap.h"

#include <string>

#include "citescoop/languages.h"

namespace proto = wikiopencite::proto;

namespace wikiopencite::citescoop::cli {

proto::Language WikipediaCodeToLanguage(const char* code, size_t len) {
  const LanguageCode* result = Languages::lookup(code, len);
  if (result) {
    return static_cast<proto::Language>(result->lang_value);
  }
  return proto::Language::LANGUAGE_UNSPECIFIED;
}

proto::Language WikipediaCodeToLanguage(const std::string& code) {
  return WikipediaCodeToLanguage(code.data(), code.size());
}

proto::Language WikipediaCodeToLanguage(const char* code) {
  return WikipediaCodeToLanguage(code, strlen(code));
}

proto::Language WikipediaDomainToLanguage(const char* domain) {
  const char* dot = strchr(domain, '.');
  if (dot) {
    return WikipediaCodeToLanguage(domain, static_cast<size_t>(dot - domain));
  }
  return proto::Language::LANGUAGE_UNSPECIFIED;
}

proto::Language WikipediaDomainToLanguage(const std::string& domain) {
  size_t dot_pos = domain.find('.');
  if (dot_pos != std::string::npos) {
    return WikipediaCodeToLanguage(domain.data(), dot_pos);
  }
  return proto::Language::LANGUAGE_UNSPECIFIED;
}
}  // namespace wikiopencite::citescoop::cli
