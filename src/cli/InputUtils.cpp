
#include "cli/InputUtils.h"

#include <charconv>
#include <cctype>
#include <stdexcept>
#include <string>
#include <system_error>

using namespace std;

namespace {

string trim(const string& value) {
    size_t first = 0;
    size_t last = value.size();

    while (first < last &&
           isspace(static_cast<unsigned char>(
               value[first]
           ))) {
        ++first;
    }

    while (last > first &&
           isspace(static_cast<unsigned char>(
               value[last - 1]
           ))) {
        --last;
    }

    return value.substr(first, last - first);
}

} // namespace

optional<int> readInteger(
    istream& input,
    ostream& output,
    const string& prompt,
    int minimum,
    int maximum
) {
    if (minimum > maximum) {
        throw invalid_argument(
            "Invalid integer input range."
        );
    }

    string line;

    while (true) {
        output << prompt;

        if (!getline(input, line)) {
            return nullopt;
        }

        line = trim(line);

        int value = 0;

        const char* begin = line.data();
        const char* end = begin + line.size();

        auto parsed = from_chars(begin, end, value);

        if (parsed.ec == errc{} &&
            parsed.ptr == end &&
            value >= minimum &&
            value <= maximum) {
            return value;
        }

        output << "Invalid input. Enter an integer from "
               << minimum << " to " << maximum
               << ".\n";
    }
}

optional<string> readText(
    istream& input,
    ostream& output,
    const string& prompt,
    size_t maxLength
) {
    string line;

    while (true) {
        output << prompt;

        if (!getline(input, line)) {
            return nullopt;
        }

        line = trim(line);

        if (!line.empty() &&
            line.size() <= maxLength) {
            return line;
        }

        output << "Invalid text. Enter between 1 and "
               << maxLength << " characters.\n";
    }
}
