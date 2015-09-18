#ifndef HTIO2_CAST_H
#define HTIO2_CAST_H

#include <sstream>

namespace htio2
{

template <typename T>
bool from_string(const std::string& input, T& output);

template <>
inline bool from_string<std::string>(const std::string& input, std::string& output)
{
    output = input;
    return true;
}

template <>
bool from_string<bool>(const std::string& input, bool& output);

// cast to signed integers
template <>
bool from_string<short>(const std::string& input, short& output);

template <>
bool from_string<int>(const std::string& input, int& output);

template <>
bool from_string<long>(const std::string& input, long& output);

template <>
bool from_string<long long>(const std::string& input, long long& output);

// cast to unsigned integers
template <>
bool from_string<unsigned short>(const std::string& input, unsigned short& output);

template <>
bool from_string<unsigned int>(const std::string& input, unsigned int& output);

template <>
bool from_string<unsigned long>(const std::string& input, unsigned long& output);

template <>
bool from_string<unsigned long long>(const std::string& input, unsigned long long& output);

// cast to floating points
template <>
bool from_string<float>(const std::string& input, float& output);

template <>
bool from_string<double>(const std::string& input, double& output);

template <>
bool from_string<long double>(const std::string& input, long double& output);

// stringify primitive types
template <typename T>
std::string to_string(T input)
{
    std::ostringstream result;
    result << input;
    return result.str();
}


} // namespace htio2

#endif // HTIO2_CAST_H
