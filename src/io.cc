#include "io.h"

#include <cstdint>
#include <istream>
#include <ostream>

#include "citescoop/io.h"
#include "citescoop/proto/file_header.pb.h"
#include "citescoop/proto/openalex/author.pb.h"
#include "citescoop/proto/openalex/institution.pb.h"
#include "citescoop/proto/openalex/work.pb.h"
#include "citescoop/proto/page.pb.h"
#include "citescoop/proto/revision.pb.h"
#include "spdlog/spdlog.h"

#include "exceptions.h"

namespace {
namespace proto = wikiopencite::proto;
namespace cs = wikiopencite::citescoop;
}  // namespace

namespace wikiopencite::citescoop::cli::io {

std::unique_ptr<PbfFile> OpenPbfFile(std::string path) {
  auto file = std::make_unique<PbfFile>();
  file->stream = std::ifstream(path, std::ios::in | std::ios::binary);
  file->reader = std::make_unique<cs::MessageReader>(&file->stream);
  return file;
}

void ClosePbfFile(std::unique_ptr<PbfFile> file) {
  file->stream.close();
}

std::unique_ptr<proto::FileHeader> ReadPbfHeader(PbfFile* file) {
  auto header = file->reader->ReadMessage<proto::FileHeader>();

  const google::protobuf::EnumDescriptor* descriptor =
      proto::FileType_descriptor();

  spdlog::trace("Got file of type {} with {} messages",
                descriptor->FindValueByNumber(header->type())->name(),
                header->count());

  if (header->type() == proto::FileType::FILE_TYPE_UNSPECIFIED) {
    spdlog::warn("File type is unspecified");
    throw exceptions::UnsupportedFileType(
        "file type not specified or pbf file corrupt");
  }

  return header;
}

std::unique_ptr<google::protobuf::Message> ReadGenericMessage(
    PbfFile* file, proto::FileType file_type) {
  switch (file_type) {
    case proto::FileType::FILE_TYPE_PAGES:
      return file->reader->ReadMessage<proto::Page>();

    case proto::FileType::FILE_TYPE_REVISIONS:
      return file->reader->ReadMessage<proto::Revision>();

    case proto::FileType::FILE_TYPE_OPENALEX_AUTHORS:
      return file->reader->ReadMessage<proto::openalex::Author>();

    case proto::FileType::FILE_TYPE_OPENALEX_INSTITUTIONS:
      return file->reader->ReadMessage<proto::openalex::Institution>();

    case proto::FileType::FILE_TYPE_OPENALEX_WORKS:
      return file->reader->ReadMessage<proto::openalex::Work>();

    default:
      const google::protobuf::EnumDescriptor* descriptor =
          proto::FileType_descriptor();
      spdlog::warn("File type {} not recognised",
                   descriptor->FindValueByNumber(file_type)->name());
      throw exceptions::UnsupportedFileType();
  }
  return std::unique_ptr<google::protobuf::Message>();
}

void PrependHeader(uint64_t message_count, proto::FileType file_type,
                   const std::istream& input, std::ostream* output) {
  auto header = proto::FileHeader();
  header.set_count(message_count);
  header.set_type(file_type);
  PrependHeader(header, input, output);
}

void PrependHeader(wikiopencite::proto::FileHeader header,
                   const std::istream& input, std::ostream* output) {
  auto writer = cs::MessageWriter(output);
  writer.WriteMessage(header);
  *output << input.rdbuf();
}
}  // namespace wikiopencite::citescoop::cli::io
