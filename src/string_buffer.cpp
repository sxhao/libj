// Copyright (c) 2012 Plenluno All rights reserved.

#include <list>

#include "libj/string_buffer.h"

namespace libj {

class StringBufferImpl : public StringBuffer {
 public:
    static Ptr create() {
        return Ptr(new StringBufferImpl());
    }

    Size length() const {
        return buf_.length();
    }

    Char charAt(Size index) const {
        if (index >= length()) {
            return NO_CHAR;
        } else {
            return buf_[index];
        }
    }

    Boolean append(const Value& val) {
        String::CPtr s = String::valueOf(val);
        if (s) {
            buf_.append(s->toStdU32String());
            return true;
        } else {
            return false;
        }
    }

    String::CPtr toString() const {
        return String::create(buf_);
    }

 private:
    std::u32string buf_;
};

StringBuffer::Ptr StringBuffer::create() {
    return StringBufferImpl::create();
}

}  // namespace libj
