#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <exception>
#include <sstream>

namespace test_framework {

struct TestCase {
    std::string name;
    std::string tag;
    std::function<void()> func;
};

class TestRegistry {
public:
    static TestRegistry& instance() {
        static TestRegistry reg;
        return reg;
    }

    void addTest(std::string name, std::string tag, std::function<void()> func) {
        tests_.push_back({std::move(name), std::move(tag), std::move(func)});
    }

    int runAll() {
        int passed = 0;
        int failed = 0;
        std::cout << "========================================================\n"
                  << " Running " << tests_.size() << " test cases...\n"
                  << "========================================================\n";

        for (const auto& test : tests_) {
            std::cout << "[ RUN      ] " << test.name << " (" << test.tag << ")\n";
            try {
                currentTestFailed_ = false;
                test.func();
                if (!currentTestFailed_) {
                    std::cout << "[       OK ] " << test.name << "\n";
                    ++passed;
                } else {
                    std::cout << "[  FAILED  ] " << test.name << "\n";
                    ++failed;
                }
            } catch (const std::exception& e) {
                std::cout << "[  FAILED  ] " << test.name << " (Exception: " << e.what() << ")\n";
                ++failed;
            } catch (...) {
                std::cout << "[  FAILED  ] " << test.name << " (Unknown exception)\n";
                ++failed;
            }
        }

        std::cout << "========================================================\n"
                  << " Test Summary: " << passed << " passed, " << failed << " failed\n"
                  << "========================================================\n";
        return failed == 0 ? 0 : 1;
    }

    void markFailed() { currentTestFailed_ = true; }

private:
    std::vector<TestCase> tests_;
    bool currentTestFailed_{false};
};

struct AutoRegister {
    AutoRegister(std::string name, std::string tag, std::function<void()> func) {
        TestRegistry::instance().addTest(std::move(name), std::move(tag), std::move(func));
    }
};

} // namespace test_framework

#define TEST_CONCAT_IMPL(a, b) a##b
#define TEST_CONCAT(a, b) TEST_CONCAT_IMPL(a, b)

#define TEST_CASE(name, tag) \
    static void TEST_CONCAT(test_func_, __LINE__)(); \
    static const ::test_framework::AutoRegister TEST_CONCAT(reg_, __LINE__)(name, tag, TEST_CONCAT(test_func_, __LINE__)); \
    static void TEST_CONCAT(test_func_, __LINE__)()

#define REQUIRE(expr) \
    do { \
        if (!(expr)) { \
            std::cerr << "  FAILED: REQUIRE(" #expr ") at " << __FILE__ << ":" << __LINE__ << "\n"; \
            ::test_framework::TestRegistry::instance().markFailed(); \
            return; \
        } \
    } while (false)

#define CHECK(expr) \
    do { \
        if (!(expr)) { \
            std::cerr << "  FAILED: CHECK(" #expr ") at " << __FILE__ << ":" << __LINE__ << "\n"; \
            ::test_framework::TestRegistry::instance().markFailed(); \
        } \
    } while (false)

#define REQUIRE_FALSE(expr) REQUIRE(!(expr))
#define CHECK_FALSE(expr) CHECK(!(expr))
#define STATIC_REQUIRE(expr) static_assert(expr, #expr)
#define SECTION(name) /* section marker */
