#include "htio2/Cast.h"
#include "htio2/StringUtil.h"

#include <stdlib.h>

using namespace std;

namespace htio2
{

string _prepare_input_(const std::string& input)
{
    size_t i_begin = input.find_first_not_of(" \t");
    return to_lower_case(input.substr(i_begin));
}

template <>
bool from_string<bool>(const std::string& input, bool& output)
{
    string input_lc = _prepare_input_(input);

    if (input_lc == "yes" || input_lc == "true" || input_lc == "y" || input_lc == "t" || input_lc == "1")
    {
        output = true;
        return true;
    }
    else if (input_lc == "no" || input_lc == "false" || input_lc == "n" || input_lc == "f" || input_lc == "0")
    {
        output = false;
        return true;
    }
    else
        return false;
}

// cast to signed integers
#define FROM_STRING_INT_IMPL(_out_type_, _func_) \
    char* p_end = 0;\
    _out_type_ tmp = _func_(input.c_str(), &p_end, 0);\
    if (p_end != input.c_str())\
    {\
        output = tmp;\
        return true;\
    }\
    else\
        return false;\

#define FROM_STRING_FLOAT_IMPL(_out_type_, _func_)\
    char* p_end = 0;\
    _out_type_ tmp = _func_(input.c_str(), &p_end);\
    if (p_end != input.c_str())\
    {\
        output = tmp;\
        return true;\
    }\
    else\
        return false;\

template <>
bool from_string<short>(const std::string& input, short& output)
{
    FROM_STRING_INT_IMPL(short, strtol);
}

template <>
bool from_string<int>(const std::string& input, int& output)
{
    FROM_STRING_INT_IMPL(int, strtol);
}

template <>
bool from_string<long>(const std::string& input, long& output)
{
    FROM_STRING_INT_IMPL(long, strtol);
}

template <>
bool from_string<long long>(const std::string& input, long long& output)
{
    FROM_STRING_INT_IMPL(long long, strtoll);
}

// cast to unsigned integers
template <>
bool from_string<unsigned short>(const std::string& input, unsigned short& output)
{
    FROM_STRING_INT_IMPL(unsigned short, strtoul);
}

template <>
bool from_string<unsigned int>(const std::string& input, unsigned int& output)
{
    FROM_STRING_INT_IMPL(unsigned int, strtoul);
}

template <>
bool from_string<unsigned long>(const std::string& input, unsigned long& output)
{
    FROM_STRING_INT_IMPL(unsigned long, strtoul);
}

template <>
bool from_string<unsigned long long>(const std::string& input, unsigned long long& output)
{
    FROM_STRING_INT_IMPL(unsigned long long, strtoull);
}

// cast to floating points
template <>
bool from_string<float>(const std::string& input, float& output)
{
    FROM_STRING_FLOAT_IMPL(float, strtof);
}

template <>
bool from_string<double>(const std::string& input, double& output)
{
    FROM_STRING_FLOAT_IMPL(double, strtod);
}

template <>
bool from_string<long double>(const std::string& input, long double& output)
{
    FROM_STRING_FLOAT_IMPL(long double, strtold);
}


} // namespace htio2
