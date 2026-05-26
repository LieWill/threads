#ifndef TEST_ASSERT_H
#define TEST_ASSERT_H

#include <iostream>
#include <string>
#include <vector>
#include <functional>

extern int failed;
extern int passed;

#define TEST_ASSERT(condition, msg) \
    do { \
        if (!(condition)) { \
            std::cerr << "FAIL: " << msg << " (" << __FILE__ << ":" << __LINE__ << ")" << std::endl; \
            failed++; \
        } else { \
            passed++; \
        } \
    } while(0)

#define TEST_ASSERT_EQUAL(expected, actual, msg) \
    do { \
        if ((expected) != (actual)) { \
            std::cerr << "FAIL: " << msg << " - Expected: " << (expected) << ", Got: " << (actual) \
                      << " (" << __FILE__ << ":" << __LINE__ << ")" << std::endl; \
            failed++; \
        } else { \
            passed++; \
        } \
    } while(0)

#define TEST_ASSERT_TRUE(condition, msg) TEST_ASSERT(condition, msg)
#define TEST_ASSERT_FALSE(condition, msg) TEST_ASSERT(!(condition), msg)

struct TestCase {
    std::string name;
    std::function<void()> func;
};

class TestRunner {
public:
    static TestRunner& instance() {
        static TestRunner runner;
        return runner;
    }

    void add_test(const std::string& suite, const std::string& name, std::function<void()> func) {
        tests.push_back({suite + "::" + name, func});
    }

    int run_all() {
        std::cout << "\n===========================================\n";
        std::cout << "         Running Tests\n";
        std::cout << "===========================================\n\n";

        for (const auto& test : tests) {
            std::cout << "Running: " << test.name << " ... " << std::flush;
            failed = 0;
            passed = 0;
            try {
                test.func();
                if (failed == 0) {
                    std::cout << "PASSED (" << passed << " assertions)\n";
                } else {
                    std::cout << "FAILED (" << failed << " failures)\n";
                }
            } catch (const std::exception& e) {
                std::cerr << "\nEXCEPTION: " << e.what() << std::endl;
                failed++;
            }
            total_failed += failed;
            total_passed += passed;
        }

        std::cout << "\n===========================================\n";
        std::cout << "         Test Results\n";
        std::cout << "===========================================\n";
        std::cout << "Total: " << (total_passed + total_failed) << " assertions\n";
        std::cout << "Passed: " << total_passed << "\n";
        std::cout << "Failed: " << total_failed << "\n";
        std::cout << "===========================================\n";

        return total_failed > 0 ? 1 : 0;
    }

private:
    std::vector<TestCase> tests;
    int total_failed = 0;
    int total_passed = 0;
};

#define TEST_CASE(suite, name) \
    static void suite##_##name(); \
    static struct suite##_##name##_registrar { \
        suite##_##name##_registrar() { \
            TestRunner::instance().add_test(#suite, #name, suite##_##name); \
        } \
    } suite##_##name##_instance_; \
    static void suite##_##name()

#endif
