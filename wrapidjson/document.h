// The MIT License (MIT)
//
// Copyright (c) 2020 hadesragon@gamil.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


#ifndef WRAPIDJSON_DOCUMENT_H_
#define WRAPIDJSON_DOCUMENT_H_

#include <string>

#include <rapidjson/document.h>

#include "value_ref.h"

namespace wrapidjson {

class DocumentWrapper
{
public:
    DocumentWrapper() : document_(std::make_shared<rapidjson::Document>()) {}
    virtual ~DocumentWrapper() = default;
protected:
    std::shared_ptr<rapidjson::Document> document_;
};

/////////////////////////////////////////////////////////////////////////////////////////////
/// Wrapper rapidjson::Document
/////////////////////////////////////////////////////////////////////////////////////////////
class Document : public DocumentWrapper, public ValueRef {
    static const size_t BUFFER_SIZE = 65536;
public:
    Document();
    explicit Document(const std::string&);
    explicit Document(const ValueRef&);

    ~Document() override = default;

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
