#include "htio2/StringUtil.h"

using namespace std;

namespace htio2
{

string to_upper_case(const std::string& input)
{
    string result(input.size(), '\0');
    for (size_t i = 0; i < input.size(); i++)
    {
        char chr = input[i];
        if ('a' <= chr && chr <= 'z')
            chr -= 'a' - 'A';
        result[i] = chr;
    }
    return result;
}

string to_lower_case(const std::string& input)
{
    string result(input.size(), '\0');
    for (size_t i = 0; i < input.size(); i++)
    {
        char chr = input[i];
        if ('A' <= chr && chr <= 'Z')
            chr += 'a' - 'A';
        result[i] = chr;
    }
    return result;
}

} // namespace htio2
