// SPDX-FileCopyrightText: 2025 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#include "progress_stream_buf.h"

#include <fstream>
#include <iostream>

namespace wikiopencite::citescoop::cli {
ProgressStreamBuf::ProgressStreamBuf(
    // NOLINTNEXTLINE(whitespace/indent_namespace)
    std::streambuf* source,
    // NOLINTNEXTLINE(whitespace/indent_namespace)
    std::streamsize total)
    // NOLINTNEXTLINE(whitespace/indent_namespace)
    : sourceBuf(source), totalBytes(total), bytesRead(0) {}

double ProgressStreamBuf::GetProgress() const {
  return totalBytes > 0 ? static_cast<double>(bytesRead) /
                              static_cast<double>(totalBytes) * 100.0
                        : 0.0;
}

std::basic_streambuf<char>::int_type ProgressStreamBuf::underflow() {
  return sourceBuf->sgetc();
}

std::basic_streambuf<char>::int_type ProgressStreamBuf::uflow() {
  int_type c = sourceBuf->sbumpc();
  if (c != traits_type::eof()) {
    bytesRead++;
  }
  return c;
}

std::streamsize ProgressStreamBuf::xsgetn(char* s, std::streamsize n) {
  std::streamsize read = sourceBuf->sgetn(s, n);
  bytesRead += read;
  return read;
}
}  // namespace wikiopencite::citescoop::cli
