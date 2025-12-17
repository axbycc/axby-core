#include <filesystem>
#include <fstream>

#if defined(_WIN32)
#include <shlobj.h>
#include <windows.h>
#else
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include "debug/check.h"
#include "debug/log.h"
#include "files.h"
#include "tools/cpp/runfiles/runfiles.h"

namespace axby {

using bazel::tools::cpp::runfiles::Runfiles;
std::unique_ptr<Runfiles> _runfiles;

void init_runfiles(const char* argv0) {
    _runfiles.reset(Runfiles::Create(argv0, BAZEL_CURRENT_REPOSITORY));
}

std::string get_rlocation(std::string_view runfiles_path) {
    CHECK(_runfiles);
    auto path = _runfiles->Rlocation(std::string(runfiles_path));
    CHECK(std::filesystem::exists(path));
    return path;
}

std::vector<std::byte> read_bytes_from_file(std::string_view path,
                                            bool* success) {
    std::ifstream file(std::string(path), std::ios::binary | std::ios::ate);

    if (!file.is_open()) {
        if (success) {
            *success = false;
            return {};
        }
        LOG(FATAL) << "Unable to open file: " << path;
    }

    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<std::byte> fileBytes(fileSize);
    if (!file.read(reinterpret_cast<char*>(fileBytes.data()), fileSize)) {
        LOG(FATAL) << "Error reading file: " << path;
    }

    if (success) {
        *success = true;
    }

    return fileBytes;
}

std::vector<std::byte> read_bytes_from_rpath(std::string_view runfiles_path) {
    auto filepath = get_rlocation(runfiles_path);
    return read_bytes_from_file(filepath);
};

void write_bytes_to_file(std::string_view path, Seq<const std::byte> bytes) {
    // need to convert to string b/c path possibly non-null terminated.
    // remove when ofstream can take string view
    std::ofstream file(std::string(path), std::ios::binary);
    if (!file) {
        LOG(FATAL) << "Error opening file for writing: " << path;
    }
    file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    if (!file) {
        LOG(FATAL) << "Error writing file: " << path;
    }
}

std::string get_home_path() {
    std::string home_path;

#if defined(_WIN32)
    char home_dir[MAX_PATH];
    CHECK(SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, home_dir)));
    home_path = home_dir;
#else
    const char* home_env = std::getenv("HOME");
    if (home_env) {
        home_path = home_env;
    } else {
        struct passwd* pw = getpwuid(getuid());
        if (pw) {
            home_path = pw->pw_dir;
        } else {
            LOG(FATAL) << "Could not get home path";
        }
    }
#endif
    return home_path;
}

std::string prepend_home_path(std::string_view path) {
    std::string home_path = get_home_path();
    if (!home_path.empty() && !path.empty() && path[0] != '/') {
        home_path += '/';
    }

    return home_path + std::string(path);
}

inline bool is_abs(std::string_view p) {
    // Very basic: Unix absolute or Windows drive absolute
    return (!p.empty() && (p[0] == '/' || p[0] == '\\')) ||
           (p.size() > 2 && std::isalpha(p[0]) && p[1] == ':' &&
            (p[2] == '\\' || p[2] == '/'));
}

std::string path_join(std::initializer_list<std::string_view> parts) {
    std::string result;

    for (auto&& p : parts) {
        if (p.empty()) continue;

        if (is_abs(p)) {
            // Reset to absolute path like Python os.path.join does
            result = std::string(p);
            continue;
        }

        if (!result.empty() && result.back() != '/' && result.back() != '\\') {
#ifdef _WIN32
            result.push_back('\\');
#else
            result.push_back('/');
#endif
        }

        result.append(p);
    }

    return result;
}

}  // namespace axby
