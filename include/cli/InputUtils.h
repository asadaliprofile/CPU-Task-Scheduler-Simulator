
#ifndef INPUT_UTILS_H
#define INPUT_UTILS_H

#include <istream>
#include <optional>
#include <ostream>
#include <string>

std::optional<int> readInteger(
    std::istream& input,
    std::ostream& output,
    const std::string& prompt,
    int minimum,
    int maximum
);

std::optional<std::string> readText(
    std::istream& input,
    std::ostream& output,
    const std::string& prompt,
    std::size_t maxLength = 24
);

#endif
