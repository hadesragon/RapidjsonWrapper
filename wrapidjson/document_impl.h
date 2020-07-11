#include <limits>
#include <memory>
#include <iostream>
#include <fstream>
#include <cstdio>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/error/en.h>
#include <rapidjson/error/error.h>

namespace wrapidjson {

/////////////////////////////////////////////////////////////////////////////////////////////
/// std::istream wrapper
/////////////////////////////////////////////////////////////////////////////////////////////
class IStream {
public:
    using Ch = char;
    IStream(std::istream& is) : is_(is) { }
    IStream(const IStream&) = delete;
    IStream& operator=(const IStream&) = delete;

    Ch Peek() const {
        int c = is_.peek();
        return c == std::char_traits<Ch>::eof() ? '\0' : static_cast<Ch>(c);
    }
    Ch Take() {
        int c = is_.get();
        return c == std::char_traits<Ch>::eof() ? '\0' : static_cast<Ch>(c);
    }
    size_t Tell() const { return static_cast<size_t>(is_.tellg()); }

    Ch* PutBegin() { throw std::runtime_error("IStream::PutBegin not implement"); }
    void Put(Ch) { throw std::runtime_error("IStream::Put not implement"); }
    void Flush() { throw std::runtime_error("IStream::Flush not implement"); }
    size_t PutEnd(Ch*) { throw std::runtime_error("IStream::PutEnd not implement"); }
private:
    std::istream& is_;
};

/////////////////////////////////////////////////////////////////////////////////////////////
/// std::ostream wrapper
/////////////////////////////////////////////////////////////////////////////////////////////
class OStream {
public:
    using Ch = char;
    OStream(std::ostream& os) : os_(os) {}
    OStream(const OStream&) = delete;
    OStream& operator=(const OStream&) = delete;

    Ch Peek() const { throw std::runtime_error("OStream::Peek not implement"); }
    Ch Take() { throw std::runtime_error("OStream::Take not implement"); }
    size_t Tell() const { throw std::runtime_error("OStream::Tell not implement"); }
    Ch* PutBegin() { throw std::runtime_error("OStream::PutBegin not implement"); }
    void Put(Ch c) { os_.put(c); }
    void Flush() { os_.flush(); }
    size_t PutEnd(Ch*) { throw std::runtime_error("OStream::PutEnd not implement"); }

private:
    std::ostream& os_;
};

inline Document::Document() 
    : DocumentBuf()
    , ValueRef(*document_, document_->GetAllocator())
{}

inline Document::Document(const std::string& buffer)
    : Document()
{
    load_from_buffer(buffer);
}

/// load JSON data
inline bool Document::load_from_file(const std::string& path) {
    FILE* fp = fopen(path.c_str(), "r");
    if (fp == nullptr) {
        return false;
    }

    char    readBuffer[BUFFER_SIZE];
    rapidjson::FileReadStream is(fp, readBuffer, BUFFER_SIZE);
    document_->ParseStream(is);
    fclose(fp);
    return not document_->HasParseError();
}

inline bool Document::load_from_buffer(const std::string& buffer) {
    document_->Parse<0>(buffer.c_str());
    return not document_->HasParseError();
}
inline bool Document::load_from_stream(std::istream& is) {
    IStream is_wrapper(is);
    document_->ParseStream(is_wrapper);
    return not document_->HasParseError();
}

inline std::string Document::get_load_error() {
    return detail::format("Error offset[%u]: %s",
            (unsigned)document_->GetErrorOffset(),
            rapidjson::GetParseError_En(document_->GetParseError()));
}

/// save JSON data
inline bool Document::save_to_file(const std::string& path, bool pretty) {
    FILE* fp = fopen(path.c_str(), "w");
    if (fp == nullptr) {
        return false;
    }

    char writeBuffer[BUFFER_SIZE];
    rapidjson::FileWriteStream os(fp, writeBuffer, BUFFER_SIZE);

    bool ret = false;
    if (pretty) {
        rapidjson::PrettyWriter<rapidjson::FileWriteStream> writer(os);
        ret = document_->Accept(writer);
    } else{
        rapidjson::Writer<rapidjson::FileWriteStream> writer(os);
        ret = document_->Accept(writer);
    }
    fclose(fp);
    return ret;
}

inline bool Document::save_to_buffer(std::string& buffer, bool pretty) {
    rapidjson::StringBuffer stringBuf;
    bool ret = false;
    if (pretty){
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(stringBuf);
        ret = document_->Accept(writer);
    } else {
        rapidjson::Writer<rapidjson::StringBuffer> writer(stringBuf);
        ret = document_->Accept(writer);
    }
    if ( ret ) {
        buffer.assign(stringBuf.GetString(), stringBuf.GetSize());
    }
    return ret;
}

inline bool Document::save_to_stream(std::ostream& os, bool pretty) {
    OStream os_wrapper(os);
    if (pretty) {
        rapidjson::PrettyWriter<OStream> writer(os_wrapper);
        return document_->Accept(writer);
    } else {
        rapidjson::Writer<OStream> writer(os_wrapper);
        return document_->Accept(writer);
    }
}

} // namespace wrapidjson
