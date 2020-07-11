#ifndef WRAPIDJSON_DOCUMENT_H_
#define WRAPIDJSON_DOCUMENT_H_

#include <string>

#include <rapidjson/document.h>

#include "value_ref.h"

namespace wrapidjson {

class DocumentBuf
{
public:
    DocumentBuf() : document_(std::make_shared<rapidjson::Document>()) {}
    virtual ~DocumentBuf() = default;
protected:
    std::shared_ptr<rapidjson::Document> document_;
};

/////////////////////////////////////////////////////////////////////////////////////////////
/// Wrapper rapidjson::Document
/////////////////////////////////////////////////////////////////////////////////////////////
class Document : public DocumentBuf, public ValueRef {
    static const size_t BUFFER_SIZE = 65536;
public:
    Document();
    explicit Document(const std::string&);

    virtual ~Document() = default;

    /// load JSON data
    bool load_from_file(const std::string& path);
    bool load_from_buffer(const std::string& buffer);
    bool load_from_stream(std::istream& is);
    std::string get_load_error();

    /// save JSON data
    bool save_to_file(const std::string& path, bool pretty = false);
    bool save_to_buffer(std::string& buffer, bool pretty = false);
    bool save_to_stream(std::ostream& os, bool pretty = false);

    /// get the actual rapidjson::Document by reference
    inline rapidjson::Document& get_document() {
        return *document_;
    }
};

} // namespace wrapidjson

#include "document_impl.h"

#endif // WRAPIDJSON_DOCUMENT_H_
