// Copyright (c) 2012 Plenluno All rights reserved.

#ifndef LIBJ_MUTABLE_H_
#define LIBJ_MUTABLE_H_

#include "libj/value.h"
#include "libj/object.h"
#include "libj/gc_base.h"

namespace libj {

class Mutable
    : public Object
    , public GCBase
    , public MutableBase {
 public:
    bool instanceOf(TypeId id) const {
        return id == Type<Mutable>::id()
            || Object::instanceOf(id);
    }
};

#define LIBJ_MUTABLE_DECLS(T, B) public: \
    typedef LIBJ_PTR(T) Ptr; \
    typedef LIBJ_CPTR(T) Cptr; \
    static Ptr create(); \
    Ptr clone() const; \
    libj::TypeId type() const { \
        return libj::Type<T>::id(); \
    } \
    bool instanceOf(libj::TypeId id) const { \
        return id == libj::Type<T>::id() \
            || B::instanceOf(id); \
    }

#define LIBJ_MUTABLE(T) public libj::Mutable { \
    LIBJ_MUTABLE_DECLS(T, libj::Mutable)

}  // namespace libj

#endif  // LIBJ_MUTABLE_H_