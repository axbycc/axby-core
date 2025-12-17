#pragma once

#define RETURN_IF_NOTEQ(expr, errcode)                                         \
    {                                                                          \
        auto result = (expr);                                                  \
        if (result != (errcode)) {                                             \
            return result;                                                     \
        }                                                                      \
    }

#define CBOR_RETURN_IF_ERROR(expr)                                             \
    { RETURN_IF_NOTEQ(expr, CborNoError); }
