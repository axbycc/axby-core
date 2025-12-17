#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "seq/seq.h"

namespace axby {

// needs to be called before rlocation related functions
// this is done by main_init in main.cpp
void init_runfiles(const char* argv0);


std::string get_rlocation(std::string_view runfiles_path);

// in the functions below, if success ptr is not supplied, the
// function will LOG(FATAL) when the file does not exist
std::vector<std::byte> read_bytes_from_file(std::string_view path,
                                            bool* success = nullptr);

std::vector<std::byte> read_bytes_from_rpath(std::string_view runfiles_path);

void write_bytes_to_file(std::string_view path, Seq<const std::byte> bytes);

// Windows Behavior:
// if path is "myfile.txt", returns
// something like C:\Users\<CurrentUserName>\myfile.txt

// Linux Behavior:
// if path is "myfile.txt", returns
// something like /home/<CurrentUserName/myfile.txt
std::string prepend_home_path(std::string_view path);
std::string get_home_path(); // C:\Users\<CurrentUserName> or /home/<CurrentUserName>

}  // namespace axby
