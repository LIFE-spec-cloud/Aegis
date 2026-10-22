#include "catch_compat.hpp"

int main() {
    return ::test_framework::TestRegistry::instance().runAll();
}
