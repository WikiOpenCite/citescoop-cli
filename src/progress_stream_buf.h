// SPDX-FileCopyrightText: 2025 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SRC_PROGRESS_STREAM_BUF_H_
#define SRC_PROGRESS_STREAM_BUF_H_
#include <fstream>
#include <iostream>

namespace wikiopencite::citescoop::cli {

class ProgressStreamBuf : public std::streambuf {
 private:
  std::streambuf* sourceBuf;
  std::streamsize totalBytes;
  std::streamsize bytesRead;

 public:
  ProgressStreamBuf(std::streambuf* source, std::streamsize total);

  double GetProgress() const;

  std::streamsize getBytesRead() const { return bytesRead; }

 protected:
  // Called for single character reads
  int_type underflow() override;

  int_type uflow() override;

  // Called for bulk reads - this is the important one for performance
  std::streamsize xsgetn(char* s, std::streamsize n) override;
};

}  // namespace wikiopencite::citescoop::cli
#endif  // SRC_PROGRESS_STREAM_BUF_H_
