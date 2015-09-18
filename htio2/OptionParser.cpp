#include "htio2/OptionParser.h"
#include "htio2/StringUtil.h"

#include <algorithm>
#include <limits>
#include <cstdio>

#if defined _WIN32 || defined _WIN64
#include <windows.h>
#else
#include <sys/ioctl.h>
#endif

using namespace std;

namespace htio2
{

size_t get_terminal_width()
{
#if defined _WIN32 || defined _WIN64
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    size_t columns = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    return columns;
#else
    struct winsize win_size;
    ioctl(fileno(stdout), TIOCGWINSZ, &win_size);
    return win_size.ws_col;
#endif
}

void _break_desc_lines_(const string& input, size_t line_size, vector<string>& output)
{
    vector<string> paragraphs;
    htio2::split(input, '\n', paragraphs);



    for (size_t line_i = 0; line_i < paragraphs.size(); line_i++)
    {
        output.push_back(string());
        const string& para = paragraphs[line_i];
        vector<string> tokens;
        htio2::split(para, ' ', tokens);

        for (size_t i_token = 0; i_token < tokens.size(); i_token++)
        {
            const string& token = tokens[i_token];
            if (output.back().size() + token.size() + 1 > line_size)
                output.push_back(string());
            if (output.back().length())
                output.back() += " ";
            output.back() += token;
        }
    }
}


ValueLimit::ValueLimit(size_t min, size_t max)
    : min(min)
    , max(max) {}

ValueLimit ValueLimit::Fixed(size_t num)
{
    return ValueLimit(num, num);
}

ValueLimit ValueLimit::Ranged(size_t min, size_t max)
{
    return ValueLimit(min, max);
}

ValueLimit ValueLimit::Free()
{
    return ValueLimit(0, numeric_limits<size_t>::max());
}

Option::ValueBase::ValueBase(ValueLimit limit): limit(limit) {}


Option::ValueBase::~ValueBase() {}

Option::BoolSwitch::BoolSwitch(bool *value)
    : ValueBase(ValueLimit(0,0))
    , value(value) {}

Option::BoolSwitch::~BoolSwitch() {}


bool Option::BoolSwitch::cast_values(const std::vector<string> &values)
{
    if (values.size())
        abort();
    *value = true;
    return true;
}

string Option::BoolSwitch::to_string()
{
    return "";
}

Option::Option(Option &&other) noexcept
    : name_long(other.name_long)
    , name_short(other.name_short)
    , group(other.group)
    , store(other.store)
    , flags(other.flags)
    , option_desc(other.option_desc)
    , value_desc(other.value_desc)
{
    other.store = nullptr;
}

Option::~Option()
{
    if (store)
        delete store;
}

string Option::format_doc(size_t w_line, size_t w_key_left, size_t w_key, size_t w_key_right) const
{
    // validate key width
    string doc_key = format_doc_key();

    int32_t w_key_remain = w_key - doc_key.size();
    if (w_key_remain < 0)
    {
        fprintf(stderr, "option key exceeds width limit %lu: %lu\n%s\n",
                w_key, doc_key.size(), doc_key.c_str());
        abort();
    }

    // get desc with default value
    string full_desc;
    string default_value_str = store->to_string();
    if (default_value_str.length())
        full_desc = "[" + default_value_str + "] " + option_desc;
    else
        full_desc = option_desc;

    // format document
    string result;
    if (w_key_left + w_key + w_key_right > w_line / 2)
    {
        // key is too wide, put desc in new line
        result = string(w_key_left, ' ') + doc_key + "\n";

        vector<string> desc_lines;
        _break_desc_lines_(full_desc, w_line - 4, desc_lines);

        for (size_t i = 0; i < desc_lines.size(); i++)
        {
            result += "    " + desc_lines[i] + "\n";
        }
    }
    else
    {
        vector<string> desc_lines;
        _break_desc_lines_(full_desc, w_line - w_key_left - w_key - w_key_right, desc_lines);

        string leading_space(w_key_left + w_key + w_key_right, ' ');

        for (size_t i = 0; i < desc_lines.size(); i++)
        {
            if (i == 0)
            {
                result += string(w_key_left, ' ') + doc_key + string(w_key_remain, ' ') + string(w_key_right, ' ');
            }
            else
            {
                result += leading_space;
            }
            result += desc_lines[i] + "\n";
        }
    }

    return result;
}


string Option::format_doc_key() const
{
    string result;
    if (name_short)
    {
        result.push_back('-');
        result.push_back(name_short);
    }
    if (name_long.length())
    {
        if (result.length())
            result += "|";
        result += "--" + name_long;
    }
    if (value_desc.length())
    {
        if (result.length())
            result += " ";
        result += value_desc;
    }
    return result;
}

bool OptionParser::add_option(Option& option)
{
    bool long_inserted = false;
    bool short_inserted = false;

    size_t idx = options.size();

    if (option.name_long.size())
    {
        long_inserted = options_by_long.insert(make_pair(option.name_long, idx)).second;
    }

    if (option.name_short)
    {
        short_inserted = options_by_short.insert(make_pair(option.name_short, idx)).second;
    }

    if (long_inserted || short_inserted)
    {
        options.push_back(&option);

        const string& group = option.group;
        size_t id = group_id_seed++;

        group_ids.insert(make_pair(group, id));

        return true;
    }
    else
    {
        return false;
    }
}

typedef pair<int, string> IndexedValue;
typedef vector<IndexedValue> IndexedValueGroup;
typedef map<Option*, vector<IndexedValueGroup> > OptionValueMap;

int _groups_sum_(const vector<IndexedValueGroup>& groups)
{
    int re = 0;
    for (size_t i = 0; i < groups.size(); i++)
    {
        re += groups[i].size();
    }
    return re;
}

inline void _dump_value_group_(OptionValueMap& store, Option* option, IndexedValueGroup& values)
{
    store[option].push_back(values);
    values.clear();
}

void OptionParser::parse_options(int& argc, char** argv)
{
    //
    // collect values by option
    //
    OptionValueMap values_by_option;
    {
        Option* curr_option = NULL;
        IndexedValueGroup curr_value_group;
        for (int i = 1; i < argc; i++)
        {
            string curr_token((argv)[i]);

            // token is option key
            if (curr_token[0] == '-' && curr_token.length() >= 2)
            {
                // long key in "--xxx" style
                if (curr_token[1] == '-')
                {
                    Option* next_option = find_option_long(curr_token);

                    if (next_option)
                    {
                        // clear previous values
                        _dump_value_group_(values_by_option, curr_option, curr_value_group);
                        curr_option = next_option;
                    }
                    else
                    {
                        curr_value_group.emplace_back(i, curr_token);
                    }
                }

                // short key in "-x" style
                else
                {
                    Option* next_option = find_option_short(curr_token[1]);

                    if (next_option)
                    {
                        // clear previous values
                        _dump_value_group_(values_by_option, curr_option, curr_value_group);
                        curr_option = next_option;

                        // short key contains a value
                        if (curr_token.length() > 2)
                        {
                            string curr_token_value = curr_token.substr(2);

                            // extra check for that the option must accept value
                            if (curr_option->store->limit.max == 0)
                            {
                                fprintf(stderr, "ERROR: option -%c is bool switch, but a value \"%s\" is given via \"%s\"\n",
                                        curr_option->name_short, curr_token_value.c_str(), curr_token.c_str());
                                exit(EXIT_FAILURE);
                            }

                            curr_value_group.emplace_back(i, curr_token_value);
                        }
                    }
                    else
                    {
                        curr_value_group.emplace_back(i, curr_token);
                    }
                }
            }
            // token is not key
            else
            {
                curr_value_group.push_back(make_pair(i, curr_token));
            }
        } // argv cycle

        // lasting...
       _dump_value_group_(values_by_option, curr_option, curr_value_group);
    }

    //
    // set option values
    //
    vector<int> unused_indices;
    for (OptionValueMap::iterator it = values_by_option.begin();
         it != values_by_option.end();
         ++it)
    {
        Option* option = it->first;
        vector<IndexedValueGroup>& value_groups = it->second;

        if (option)
        {
            vector<string> values_plain;

            // a bool switch option, accept no values
            if (option->store->limit.max == 0)
            {
                // it should appear only once
                if (value_groups.size() > 1)
                {
                    fprintf(stderr, "ERROR: bool switch option \"%s\" appeared more than once\n",
                            option->format_doc_key().c_str());
                    exit(EXIT_FAILURE);
                }

                // all values are unused
                for (size_t i_group = 0; i_group < value_groups.size(); i_group++)
                {
                    const IndexedValueGroup& value_group = value_groups[i_group];
                    for (size_t i_value = 0; i_value < value_group.size(); i_value++)
                    {
                        unused_indices.push_back(value_group[i_value].first);
                    }
                }
            }
            // accept one value
            else if (option->store->limit.min == 1 && option->store->limit.max == 1)
            {
                // it should appear only once
                if (value_groups.size() > 1)
                {
                    fprintf(stderr, "ERROR: single value option \"%s\" appeared more than once\n",
                            option->format_doc_key().c_str());
                    exit(EXIT_FAILURE);
                }

                const IndexedValueGroup& first_group = value_groups[0];

                // we should have value
                if (!first_group.size())
                {
                    fprintf(stderr, "ERROR: single value option \"%s\" has no value\n",
                            option->format_doc_key().c_str());
                    exit(EXIT_FAILURE);
                }

                // first value is used, all following values are unused
                for (size_t i_value = 0; i_value < first_group.size(); i_value++)
                {
                    if (i_value == 0)
                        values_plain.push_back(first_group[i_value].second);
                    else
                        unused_indices.push_back(first_group[i_value].first);
                }
            }
            // accept variable number of values
            else
            {
                const char* TEMPLATE_LOWER_THAN_MIN = 0;
                if (option->store->limit.min == option->store->limit.max)
                    TEMPLATE_LOWER_THAN_MIN = "ERROR: option \"%s\" requires %lu values, but only %lu got\n";
                else
                    TEMPLATE_LOWER_THAN_MIN = "ERROR: option \"%s\" requires %lu values at least, but only %lu got\n";

                // option can occur multiple times
                if (option->flags & Option::FLAG_MULTI_KEY)
                {
                    for (size_t i_group = 0; i_group < value_groups.size(); i_group++)
                    {
                        const IndexedValueGroup& value_group = value_groups[i_group];
                        for (size_t i_value = 0; i_value < value_group.size(); i_value++)
                        {
                            if (values_plain.size() < option->store->limit.max)
                                values_plain.push_back(value_group[i_value].second);
                            else
                                unused_indices.push_back(value_group[i_value].first);
                        }
                    }

                    // validate value number
                    if (values_plain.size() < option->store->limit.min)
                    {
                        fprintf(stderr, TEMPLATE_LOWER_THAN_MIN,
                                option->format_doc_key().c_str(), option->store->limit.min, values_plain.size());
                        exit(EXIT_FAILURE);
                    }
                }
                // option can occur only once
                else
                {
                    if (value_groups.size() > 1)
                    {
                        fprintf(stderr, "ERROR: multi value option \"%s\" appeared more than once\n",
                                option->format_doc_key().c_str());
                        exit(EXIT_FAILURE);
                    }

                    const IndexedValueGroup& first_group = value_groups[0];

                    // validate value number
                    if (first_group.size() < option->store->limit.min)
                    {

                        fprintf(stderr, TEMPLATE_LOWER_THAN_MIN,
                                option->format_doc_key().c_str(), option->store->limit.min, first_group.size());
                        exit(EXIT_FAILURE);
                    }

                    // store
                    for (size_t i_value = 0; i_value < first_group.size(); i_value++)
                    {
                        if (i_value < option->store->limit.max)
                            values_plain.push_back(first_group[i_value].second);
                        else
                            unused_indices.push_back(first_group[i_value].first);
                    }
                }
            }

            if (!option->store->cast_values(values_plain))
            {
                fprintf(stderr, "ERROR: failed to cast values for option \"%s\"\n",
                        option->format_doc_key().c_str());
                for (size_t i = 0; i < values_plain.size(); i++)
                {
                    fprintf(stderr, "ERROR:   \"%s\"\n",
                            values_plain[i].c_str());
                }
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            // ungrouped options, collect to unused
            for (size_t i_group = 0; i_group < value_groups.size(); i_group++)
            {
                const IndexedValueGroup& value_group = value_groups[i_group];
                for (size_t i_value = 0; i_value < value_group.size(); i_value++)
                {
                    unused_indices.push_back(value_group[i_value].first);
                }
            }
        }
    } // option cycle

    // modify argument
    // collect unprocessed names
    vector<char*> buffer;
    for (size_t i = 0; i < unused_indices.size(); i++)
    {
        int index = unused_indices[i];
        if (index > 0)
            buffer.push_back(argv[index]);
    }

    // modify argv
    for (size_t i = 1; i < argc; i++)
    {
        if (i <= buffer.size())
        {
            argv[i] = buffer[i-1];
        }
        else
        {
            argv[i] = NULL;
        }
    }

    argc = buffer.size() + 1;
}

Option* OptionParser::find_option_long(const string &key)
{
    string key_use = key.substr(2);

    // find exact key
    map<string, size_t>::iterator it = options_by_long.find(key_use);
    if (it != options_by_long.end())
        return options[it->second];

    // find by prefix
    vector<string> matched_keys;
    vector<size_t> matched_i;

    for (it = options_by_long.begin(); it != options_by_long.end(); it++)
    {
        const string& curr_key = it->first;

        if (curr_key.find(key_use) == 0)
        {
            matched_keys.push_back("--"+curr_key);
            matched_i.push_back(it->second);
        }
    }

    if (matched_keys.size() == 0)
    {
        return nullptr;
    }
    else if (matched_keys.size() == 1)
    {
        return options[matched_i[0]];
    }
    else
    {
        fprintf(stderr, "ERROR: ambiguous option prefix \"%s\", which matches:\n", key.c_str());
        for (size_t i = 0; i < matched_keys.size(); i++)
            fprintf(stderr, "ERROR:   %s\n", matched_keys[i].c_str());
        exit(EXIT_FAILURE);
    }
}

Option* OptionParser::find_option_short(char key)
{
    std::map<char, size_t>::iterator it = options_by_short.find(key);
    if (it != options_by_short.end())
        return options[it->second];
    else
    {
        return nullptr;
    }
}

struct GroupSort
{
    GroupSort(std::map<std::string, size_t>& ids): ids(ids) {}

    bool operator () (const string& a, const string& b)
    {
        return ids[a] < ids[b];
    }

    std::map<std::string, size_t>& ids;
};

string OptionParser::format_document()
{
    size_t line_width = get_terminal_width();

    // categorize by group
    map<string, vector<Option*> > groups;
    for (size_t i=0; i<options.size(); i++)
    {
        const string& group_name = options[i]->group;
        groups[group_name].push_back(options[i]);
    }

    string result;

    // get key length
    size_t max_doc_key_len = 0;

    for (size_t i = 0; i < options.size(); i++)
    {
        Option* opt = options[i];
        string doc_key = opt->format_doc_key();
        if (max_doc_key_len < doc_key.length())
            max_doc_key_len = doc_key.length();
    }

    // sort groups by their occurance
    vector<string> group_names;
    for (pair<string, size_t> value : group_ids)
        group_names.push_back(value.first);

    sort(group_names.begin(), group_names.end(), GroupSort(group_ids));

    // format document for each option group
    for (const string& group_name : group_names)
    {
        vector<Option*>& group_options = groups[group_name];

        if (group_name.length())
            result += group_name + "\n\n";

        for (size_t i = 0; i < group_options.size(); i++)
        {
            Option* opt = group_options[i];
            result += opt->format_doc(line_width, 2, max_doc_key_len, 2);
            result += "\n";
        }
    }

    return result;
}

} // namespace tcrk2
