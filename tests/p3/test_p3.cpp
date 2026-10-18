// tests/p3/test_p3.cpp — YOUR P3 TEST SUITE.
//
// The five tests below are worked examples. Keep them, and add your own
// until you have AT LEAST 15 (spec §6 lists what must be covered).
// Name every test <area>_<what>: reg_ for the registry, ws_ for the
// workspace, calc_ for the calculator.
//
// Build & run:   cmake --build build && ./build/test_p3
// One area:      ./build/test_p3 reg_        (names starting with "reg_")
// One test:      ./build/test_p3 =reg_add_and_find

#include "../mini_test.h"
#include "../test_helpers.h"     // TempDir, write_text, args1, ...

#include "tools/calculator.h"
#include "tools/tool_registry.h"
#include "tools/workspace.h"

#include <cstdio>
#include <memory>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

// A tool whose name you choose — use it to fill the registry in tests.
class NamedTool : public Tool {
public:
    explicit NamedTool(std::string n) : n_(std::move(n)), schema_(JsonValue::object()) {}
    std::string name() const override { return n_; }
    std::string description() const override { return "test tool"; }
    const JsonValue& schema() const override { return schema_; }
    ToolResult invoke(const JsonValue&, ToolContext&) override { return ToolResult::success(n_); }
private:
    std::string n_;
    JsonValue   schema_;   // an empty JSON object: {}
};

// Tokenize with the provided tokenizer, then run YOUR calc_evaluate.
bool eval(const std::string& expr, double* value, std::string* error) {
    std::vector<CalcToken> tokens;
    if (!calc_tokenize(expr, &tokens, error)) return false;
    return calc_evaluate(tokens, value, error);
}

}  // namespace

// ---------------------------------------------------------------- examples

TEST(reg_add_and_find) {
    ToolRegistry r;
    auto t = std::make_unique<NamedTool>("alpha");
    Tool* raw = t.get();                        // remember the address before moving
    REQUIRE(r.add(std::move(t)));
    CHECK(r.size() == 1);
    CHECK(r.find("alpha") == raw);              // same object, not a copy
    CHECK(r.find("beta") == nullptr);
}

TEST(reg_duplicate_keeps_original) {
    ToolRegistry r;
    auto first = std::make_unique<NamedTool>("alpha");
    Tool* original = first.get();
    REQUIRE(r.add(std::move(first)));
    CHECK(!r.add(std::make_unique<NamedTool>("alpha")));
    CHECK(r.find("alpha") == original);
    CHECK(r.size() == 1);
}

TEST(ws_rejects_parent_escape) {
    TempDir d;
    Workspace ws(d.path());
    fs::path out;
    std::string reason;
    REQUIRE(ws.resolve("notes/todo.txt", &out, &reason));   // sanity: accepts *something*
    CHECK(out == ws.root() / "notes" / "todo.txt");
    CHECK(!ws.resolve("../etc/passwd", &out, &reason));
    CHECK(!reason.empty());
}

TEST(calc_precedence) {
    double v = 0;
    std::string err;
    REQUIRE(eval("2 + 3 * 4", &v, &err));
    CHECK(v == 14);
}

TEST(calc_division_by_zero) {
    double v = 0;
    std::string err;
    CHECK(!eval("1 / 0", &v, &err));
    CHECK(err == "division by zero");
}

// ------------------------------------------------------------- your tests
//
// Still to write (spec §6 has the full list). Suggested names:
//   reg_null_rejected, reg_for_each_visits_each_once
//   reg_10000_tools               REQUIRED: print bucket_count / load_factor /
//                                 max_probe_length, check the bound
//   reg_alternative_hash          REQUIRED: longest chain of a weak hash
//   ws_<case>                     REQUIRED: every row of the table in spec §4.4
//   calc_<case>                   left-to-right, parentheses, negatives

// add() with a nullptr should fail, size stays 0
TEST(reg_null_rejected) {
    ToolRegistry r;
    std::unique_ptr<Tool> nothing = nullptr;
    CHECK(!r.add(std::move(nothing)));
    CHECK(r.size() == 0);
}

// looking up a name that was never added returns nullptr
TEST(reg_find_missing) {
    ToolRegistry r;
    CHECK(r.find("nothing_here") == nullptr);
    r.add(std::make_unique<NamedTool>("alpha"));
    CHECK(r.find("still_nothing") == nullptr);
}

// should go up by one each real add, stay same on dupes
TEST(reg_size_tracks_adds) {
    ToolRegistry r;
    CHECK(r.size() == 0);
    r.add(std::make_unique<NamedTool>("a"));
    CHECK(r.size() == 1);
    r.add(std::make_unique<NamedTool>("b"));
    CHECK(r.size() == 2);
    r.add(std::make_unique<NamedTool>("a")); // dupe, should not count
    CHECK(r.size() == 2);
}

// add a few tools, for_each should hit each one exactly once
TEST(reg_for_each_visits_each_once) {
    ToolRegistry r;
    r.add(std::make_unique<NamedTool>("a"));
    r.add(std::make_unique<NamedTool>("b"));
    r.add(std::make_unique<NamedTool>("c"));

    int count = 0;
    std::vector<std::string> seen;
    r.for_each([&](const Tool& t) {
        ++count;
        seen.push_back(t.name());
    });

    CHECK(count == 3);
    // make sure all three names showed up somewhere
    bool has_a = false, has_b = false, has_c = false;
    for (const std::string& n : seen) {
        if (n == "a") has_a = true;
        if (n == "b") has_b = true;
        if (n == "c") has_c = true;
    }
    CHECK(has_a && has_b && has_c);
}

// required scale test
TEST(reg_10000_tools) {
    ToolRegistry r;
    std::vector<Tool*> pointers; // remember the first pointer we got for each name

    for (int i = 0; i < 10000; i++) {
        std::string name = "tool_" + std::to_string(i);
        REQUIRE(r.add(std::make_unique<NamedTool>(name)));
        CHECK(r.load_factor() <= 1.0);
    }

    // grab the pointer for tool_0 now, after all inserts/resizes are done
    Tool* first_tool_ptr_after = r.find("tool_0");

    // every name should be findable
    for (int i = 0; i < 10000; i++) {
        std::string name = "tool_" + std::to_string(i);
        CHECK(r.find(name) != nullptr);
    }

    CHECK(r.size() == 10000);
    CHECK(r.max_probe_length() <= 64);

    std::printf("bucket_count: %zu\n", r.bucket_count());
    std::printf("load_factor: %f\n", r.load_factor());
    std::printf("max_probe_length: %zu\n", r.max_probe_length());

    (void)first_tool_ptr_after;
}

// pointer from an early add should still be valid/same after many more adds (resizes happen)
TEST(reg_pointer_stability) {
    ToolRegistry r;
    auto t = std::make_unique<NamedTool>("first");
    Tool* original_ptr = t.get();
    REQUIRE(r.add(std::move(t)));

    // add a bunch more to force resizes
    for (int i = 0; i < 1000; i++) {
        r.add(std::make_unique<NamedTool>("extra_" + std::to_string(i)));
    }

    CHECK(r.find("first") == original_ptr);
}

// show how badly a weak hash (s[0] + s.size()) would do on the same 10000 names compared to our real hash at our final bucket count
TEST(reg_alternative_hash) {
    ToolRegistry r;
    for (int i = 0; i < 10000; i++) {
        r.add(std::make_unique<NamedTool>("tool_" + std::to_string(i)));
    }

    std::size_t buckets = r.bucket_count();
    std::vector<int> weak_counts(buckets, 0);

    for (int i = 0; i < 10000; i++) {
        std::string name = "tool_" + std::to_string(i);
        // the weak hash
        std::size_t weak_hash = (unsigned char)name[0] + name.size();
        std::size_t idx = weak_hash % buckets;
        weak_counts[idx]++;
    }

    int longest_weak_chain = 0;
    for (int c : weak_counts) {
        if (c > longest_weak_chain) longest_weak_chain = c;
    }

    std::printf("real hash max_probe_length: %zu\n", r.max_probe_length());
    std::printf("weak hash longest chain:    %d\n", longest_weak_chain);

    // the weak hash should be way worse than ours
    CHECK(longest_weak_chain > (int)r.max_probe_length());
}

// ws tests
TEST(ws_accepts_dot_slash_double_slash) {
    TempDir d;
    Workspace ws(d.path());
    fs::path out;
    std::string reason;
    REQUIRE(ws.resolve("./notes//todo.txt", &out, &reason));
    CHECK(out == ws.root() / "notes" / "todo.txt");
}

TEST(ws_accepts_dot_as_root) {
    TempDir d;
    Workspace ws(d.path());
    fs::path out;
    std::string reason;
    REQUIRE(ws.resolve(".", &out, &reason));
    CHECK(out == ws.root());
}

TEST(ws_accepts_a_dotdot_b) {
    TempDir d;
    Workspace ws(d.path());
    fs::path out;
    std::string reason;
    REQUIRE(ws.resolve("a/../b", &out, &reason));
    CHECK(out == ws.root() / "b");
}

TEST(ws_accepts_a_dotdot_alone) {
    TempDir d;
    Workspace ws(d.path());
    fs::path out;
    std::string reason;
    REQUIRE(ws.resolve("a/..", &out, &reason));
    CHECK(out == ws.root());
}

TEST(ws_accepts_nonexistent_nested_path) {
    TempDir d;
    Workspace ws(d.path());
    fs::path out;
    std::string reason;
    REQUIRE(ws.resolve("new/dir/file.txt", &out, &reason));
    CHECK(out == ws.root() / "new" / "dir" / "file.txt");
}

TEST(ws_accepts_dotdot_inside_filename) {
    TempDir d;
    Workspace ws(d.path());
    fs::path out;
    std::string reason;
    REQUIRE(ws.resolve("my..notes.txt", &out, &reason));
    CHECK(out == ws.root() / "my..notes.txt");
}

TEST(ws_rejects_empty_path) {
    TempDir d;
    Workspace ws(d.path());
    fs::path out;
    std::string reason;
    CHECK(!ws.resolve("", &out, &reason));
    CHECK(!reason.empty());
}

TEST(ws_rejects_nul_byte) {
    TempDir d;
    Workspace ws(d.path());
    fs::path out;
    std::string reason;
    std::string bad_path = std::string("notes") + '\0' + "/x";
    CHECK(!ws.resolve(bad_path, &out, &reason));
}

TEST(ws_rejects_absolute_path) {
    TempDir d;
    Workspace ws(d.path());
    fs::path out;
    std::string reason;
    CHECK(!ws.resolve("/etc/passwd", &out, &reason));
}

TEST(ws_rejects_dotdot_alone) {
    TempDir d;
    Workspace ws(d.path());
    fs::path out;
    std::string reason;
    CHECK(!ws.resolve("..", &out, &reason));
}

TEST(ws_rejects_relative_dotdot_climb) {
    TempDir d;
    Workspace ws(d.path());
    fs::path out;
    std::string reason;
    CHECK(!ws.resolve("a/../../b", &out, &reason));
}

// calc tests
TEST(calc_left_to_right_minus) {
    double v = 0;
    std::string err;
    REQUIRE(eval("10 - 4 - 3", &v, &err));
    CHECK(v == 3);
}

TEST(calc_left_to_right_divide) {
    double v = 0;
    std::string err;
    REQUIRE(eval("100 / 10 / 5", &v, &err));
    CHECK(v == 2);
}

TEST(calc_parens_basic) {
    double v = 0;
    std::string err;
    REQUIRE(eval("(2 + 3) * 4", &v, &err));
    CHECK(v == 20);
}

TEST(calc_mixed_parens) {
    double v = 0;
    std::string err;
    REQUIRE(eval("2 * (3 + 4) - 5", &v, &err));
    CHECK(v == 9);
}

TEST(calc_negative_times_negative) {
    double v = 0;
    std::string err;
    REQUIRE(eval("-3 * -3", &v, &err));
    CHECK(v == 9);
}

TEST(calc_decimal_result) {
    double v = 0;
    std::string err;
    REQUIRE(eval("7 / 2", &v, &err));
    CHECK(v == 3.5);
}

// some more
TEST(calc_simple_addition) {
    double v = 0;
    std::string err;
    REQUIRE(eval("5 + 5", &v, &err));
    CHECK(v == 10);
}

TEST(calc_nested_parens) {
    double v = 0;
    std::string err;
    REQUIRE(eval("((1 + 2) * (3 + 4))", &v, &err));
    CHECK(v == 21);
}



int main(int argc, char** argv) { return mt::run_all(argc, argv); }