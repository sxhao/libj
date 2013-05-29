// Copyright (c) 2012-2013 Plenluno All rights reserved.

#ifndef LIBJ_DETAIL_JS_REGEXP_H_
#define LIBJ_DETAIL_JS_REGEXP_H_

#include <libj/endian.h>
#include <libj/glue/regexp.h>
#include <libj/detail/js_object.h>

#include <assert.h>
#include <string>
#include <vector>

namespace libj {
namespace detail {

template<typename I>
class JsRegExp : public JsObject<I> {
 public:
    JsRegExp(String::CPtr pattern, UInt flags)
        : pattern_(pattern)
        , re_(NULL) {
        if (pattern) {
            re_ = glue::RegExp::create(toStdString(pattern), flags);
        }
        setLastIndex(0);
    }

    virtual ~JsRegExp() {
        delete re_;
    }

    Boolean valid() const {
        return !!re_;
    }

    virtual Boolean global() const {
        return re_ && re_->global();
    }

    virtual Boolean ignoreCase() const {
        return re_ && re_->ignoreCase();
    }

    virtual Boolean multiline() const {
        return re_ && re_->multiline();
    }

    virtual String::CPtr source() const {
        return pattern_;
    }

    virtual Size lastIndex() const {
        LIBJ_STATIC_SYMBOL_DEF(symLastIndex, "lastIndex");
        Size lastIndex = 0;
        to<Size>(this->get(symLastIndex), &lastIndex);
        return lastIndex;
    }

    virtual JsArray::Ptr exec(String::CPtr str) {
        LIBJ_STATIC_SYMBOL_DEF(symIndex, "index");
        LIBJ_STATIC_SYMBOL_DEF(symInput, "input");

        if (!str) {
            return execFail();
        }

        Size lastIndex = 0;
        Boolean global = this->global();
        if (global) {
            lastIndex = this->lastIndex();
            if (lastIndex > str->length()) {
                return execFail();
            } else {
                str = str->substring(lastIndex);
            }
        }

        std::vector<int> captures;
        if (re_->execute(toStdString(str), 0, captures)) {
            if (global) setLastIndex(lastIndex + (captures[1] - captures[0]));
        } else {
            return execFail();
        }

        JsArray::Ptr res = JsArray::create();
        Size len = captures.size();
        assert(len > 0);
        for (Size i = 0; i < len; i += 2) {
            if (captures[i] >= 0 &&
                captures[i+1] >= 0 &&
                captures[i] <= captures[i+1] &&
                captures[i+1] <= static_cast<int>(str->length())) {
                res->add(str->substring(captures[i], captures[i+1]));
            } else {
                res->add(UNDEFINED);
            }
        }
        res->setProperty(symInput, str);
        res->setProperty(symIndex, captures[0]);
        return res;
    }

    virtual Boolean test(String::CPtr str) {
        return !!exec(str);
    }

 private:
    static std::string toStdString(String::CPtr s) {
        static Boolean big = endian() == BIG;
        assert(s);
        glue::RegExp::Encoding enc = glue::RegExp::encoding();
        switch (enc) {
        case glue::RegExp::UTF8:
            return s->toStdString();
        case glue::RegExp::UTF16:
            if (big) {
                return s->toStdString(String::UTF16BE);
            } else {
                return s->toStdString(String::UTF16LE);
            }
        case glue::RegExp::UTF32:
            if (big) {
                return s->toStdString(String::UTF16BE);
            } else {
                return s->toStdString(String::UTF16LE);
            }
        default:
            assert(false);
            return s->toStdString();
        }
    }

    void setLastIndex(Size lastIndex) {
        LIBJ_STATIC_SYMBOL_DEF(symLastIndex, "lastIndex");
        this->put(symLastIndex, lastIndex);
    }

    JsArray::Ptr execFail() {
        if (global()) setLastIndex(0);
        return JsArray::null();
    }

 private:
    String::CPtr pattern_;
    glue::RegExp* re_;
};

}  // namespace detail
}  // namespace libj

#endif  // LIBJ_DETAIL_JS_REGEXP_H_
