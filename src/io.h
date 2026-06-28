#ifndef SRC_IO_H_
#define SRC_IO_H_

#include <cstdint>
#include <fstream>
#include <istream>
#include <memory>
#include <ostream>

#include "citescoop/io.h"
#include "citescoop/proto/file_header.pb.h"

namespace wikiopencite::citescoop::cli::io {

struct PbfFile {
  std::ifstream stream;
  std::unique_ptr<wikiopencite::citescoop::MessageReader> reader;
};

std::unique_ptr<PbfFile> OpenPbfFile(std::string path);

void ClosePbfFile(std::unique_ptr<PbfFile> file);

std::unique_ptr<wikiopencite::proto::FileHeader> ReadPbfHeader(PbfFile* file);

std::unique_ptr<google::protobuf::Message> ReadGenericMessage(
    PbfFile* file, wikiopencite::proto::FileType file_type);

void PrependHeader(uint64_t message_count,
                   wikiopencite::proto::FileType file_type,
                   const std::istream& input, std::ostream* output);
void PrependHeader(wikiopencite::proto::FileHeader header,
                   const std::istream& input, std::ostream* output);
}  // namespace wikiopencite::citescoop::cli::io

#endif  // SRC_IO_H_
