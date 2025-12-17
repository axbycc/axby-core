#pragma once

#include <string>
#include <optional>

#ifndef CHECK
#include "absl/log/check.h"
#endif

const std::string __ERROR_MESSAGE_OR_EMPTY_MESSAGE__ = "empty";

struct ErrorMessage {
    ErrorMessage(std::string_view msg) : message(msg) {}
    std::string message;
};

template <typename T>
class ErrorMessageOr {
   public:
    // check valid() before calling
    const T& result() const {
        CHECK(result_);
        return *result_;
    };

    bool valid() const { return result_.has_value(); }

    // initialized means that either an error or a result was
    // explicitly set. the default constructor only sets an implicit
    // error: "empty".
    bool initialized() const { return valid() || !error_msg_.empty(); }

    // nonempty signals error, in which case caller should log the
    // error and gracefully terminate
    const std::string& error_msg() const {
        if (!result_ && error_msg_.empty()) return __ERROR_MESSAGE_OR_EMPTY_MESSAGE__;
        return error_msg_;
    }

    static ErrorMessageOr Error(const std::string& e) {
        CHECK(!e.empty());
        ErrorMessageOr out;
        out.error_msg_ = e;
        return out;
    }

    static ErrorMessageOr Error(const std::stringstream& e) {
        return Error(e.str());
    }

    ErrorMessageOr(const T& t) { result_ = t; }

    ErrorMessageOr(){};

    ErrorMessageOr(const ErrorMessage& message) {
        this->error_msg_ = message.message;
    }

   private:
    std::optional<T> result_;
    std::string error_msg_;
};
