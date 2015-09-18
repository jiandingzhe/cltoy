#ifndef HTIO2_STRING_UTIL_H
#define HTIO2_STRING_UTIL_H

#include <string>
#include <vector>

namespace htio2
{

#define STRINGIFY(content) #content
#define EXPAND_AND_STRINGIFY(input) STRINGIFY(input)

std::string to_upper_case(const std::string& input);
std::string to_lower_case(const std::string& input);

template <typename iterator_t>
std::string join(const std::string& delim, iterator_t input_begin, iterator_t input_end)
{
    std::string result;
    for (iterator_t it = input_begin; it != input_end; it++)
    {
        if (it != input_begin)
            result += delim;
        result += *it;
    }
    return result;
}

template <typename container_t>
void split(const std::string& input, char delim, container_t& output)
{
    if (!input.length())
        return;

    size_t search_from = 0;
    for (;;)
    {
        size_t i_delim = input.find(delim, search_from);

        if (i_delim == std::string::npos)
        {
            output.push_back(input.substr(search_from));
            break;
        }
        else
        {
            output.push_back(input.substr(search_from, i_delim - search_from));
            search_from = i_delim + 1;
        }
    }
}

} // namespace htio2

#endif // HTIO2_STRING_UTIL_H
