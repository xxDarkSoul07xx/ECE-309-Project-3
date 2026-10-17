// src/tools/workspace.cpp — YOU IMPLEMENT (spec §4.4).
//
// Workspace::resolve: a purely lexical check, steps 1–6 of the spec.
// Do not touch the filesystem here (no exists(), canonical(), ...).
// The constructor, root() and check_symlinks() are provided in
// workspace_provided.cpp.

#include "tools/workspace.h"

namespace fs = std::filesystem;

bool Workspace::resolve(std::string_view user_path, fs::path* out, std::string* reason) const {
    // reject empty path
    if (user_path.empty()) {
        *reason = "empty path";
        return false;
    }

    // reject if it has a nul byte hiding in it
    for (char c : user_path) {
        if (c == '\0') {
            *reason = "path contains nul byte";
            return false;
        }
    }

    // turn it into a real path object
    fs::path p{std::string(user_path)};

    // reject absolute paths bc only relative allowed
    if (p.is_absolute() || p.has_root_path()) {
        *reason = "absolute paths not allowed";
        return false;
    }

    // normalize it
    fs::path n = p.lexically_normal();

    // check the first part of the normalized path and if it starts with .. that means it tries to escape the root folder
    if (!n.empty() && *n.begin() == "..") {
        *reason = "path escapes workspace root";
        return false;
    }

    // if it just resolves to . then its the root
    if (n == ".") {
        *out = root();
    } else {
        *out = root() / n;
    }

    return true;
}