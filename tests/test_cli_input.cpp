
#include "cli/InputUtils.h"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace std;

int main() {
    int passed = 0;
    int failed = 0;

    auto check = [&](bool condition, const string& name) {
        cout << (condition ? "[PASS] " : "[FAIL] ")
             << name << '\n';

        if (condition)
            ++passed;
        else
            ++failed;
    };

    // Test 1: Valid integer.
    {
        istringstream input("5\n");
        ostringstream output;

        auto value = readInteger(
            input, output, "Number: ", 0, 10
        );

        check(
            value && *value == 5,
            "Reads valid integer"
        );
    }

    // Test 2: Invalid text followed by valid integer.
    {
        istringstream input("abc\n7\n");
        ostringstream output;

        auto value = readInteger(
            input, output, "Number: ", 0, 10
        );

        check(
            value && *value == 7 &&
            output.str().find("Invalid input") !=
                string::npos,
            "Rejects nonnumeric input and retries"
        );
    }

    // Test 3: Out-of-range value.
    {
        istringstream input("99\n3\n");
        ostringstream output;

        auto value = readInteger(
            input, output, "Number: ", 0, 10
        );

        check(
            value && *value == 3,
            "Rejects out-of-range integer"
        );
    }

    // Test 4: Trailing invalid characters.
    {
        istringstream input("4hello\n4\n");
        ostringstream output;

        auto value = readInteger(
            input, output, "Number: ", 0, 10
        );

        check(
            value && *value == 4 &&
            output.str().find("Invalid input") !=
                string::npos,
            "Rejects partially numeric input"
        );
    }

    // Test 5: Whitespace trimming.
    {
        istringstream input("   8   \n");
        ostringstream output;

        auto value = readInteger(
            input, output, "Number: ", 0, 10
        );

        check(
            value && *value == 8,
            "Trims whitespace around integers"
        );
    }

    // Test 6: Valid process name.
    {
        istringstream input("  Worker A  \n");
        ostringstream output;

        auto name = readText(
            input, output, "Name: ", 24
        );

        check(
            name && *name == "Worker A",
            "Reads and trims process names"
        );
    }

    // Test 7: Empty and oversized process names.
    {
        istringstream input(
            "   \n"
            "ThisProcessNameIsFarTooLong\n"
            "P4\n"
        );
        ostringstream output;

        auto name = readText(
            input, output, "Name: ", 10
        );

        check(
            name && *name == "P4",
            "Rejects empty and oversized names"
        );
    }

    // Test 8: End of input.
    {
        istringstream input("");
        ostringstream output;

        auto value = readInteger(
            input, output, "Number: ", 0, 10
        );

        check(
            !value.has_value(),
            "Handles end-of-input gracefully"
        );
    }

    cout << "\n===== CLI INPUT TEST SUMMARY =====\n";
    cout << "Passed: " << passed << '\n';
    cout << "Failed: " << failed << '\n';

    return failed == 0 ? 0 : 1;
}
