#ifndef HTIO2_OPTION_PARSER
#define HTIO2_OPTION_PARSER

#include <string>
#include <vector>
#include <map>

#include <cstdio>
#include <cstdlib>
#include <stdint.h>

#include "htio2/RefCounted.h"
#include "htio2/Cast.h"

namespace htio2
{

class OptionParser;
class Option;

class ValueLimit
{
    friend class OptionParser;
    friend class Option;

    ValueLimit(size_t min, size_t max);

public:
    static ValueLimit Fixed(size_t num);
    static ValueLimit Ranged(size_t min, size_t max);
    static ValueLimit Free();

protected:
    size_t min;
    size_t max;
};

class Option
{
    friend class OptionParser;

public:
    enum OptionFlag
    {
        FLAG_NONE = 0,
        FLAG_MULTI_KEY = 1,
    };

protected:
    struct ValueBase
    {
        ValueBase(ValueLimit limit);
        virtual ~ValueBase();
        virtual bool cast_values(const std::vector<std::string>& values) = 0;
        virtual std::string to_string() = 0;

        ValueLimit limit;
    };

    struct BoolSwitch: public ValueBase
    {
        BoolSwitch(bool* value);
        virtual ~BoolSwitch();

        virtual bool cast_values(const std::vector<std::string>& values);
        virtual std::string to_string();

        bool* value;
    };

    template<typename T>
    struct SingleValue: public ValueBase
    {
        SingleValue(T* value)
            : ValueBase(ValueLimit(1,1))
            , value(value) {}
        virtual ~SingleValue() {}

        virtual bool cast_values(const std::vector<std::string>& values)
        {
            if (values.size() != 1)
                abort();
            return htio2::from_string<T>(values[0], *value);
        }

        virtual std::string to_string()
        {
            return htio2::to_string<T>(*value);
        }


        T* value;
    };

    template<typename T>
    struct MultiValue: public ValueBase
    {
        MultiValue(std::vector<T>* values, ValueLimit limit)
            : ValueBase(limit)
            , value_list(values) {}


        virtual bool cast_values(const std::vector<std::string>& values)
        {
            for (size_t i = 0; i < values.size(); i++)
            {
                T tmp;
                if (!from_string<T>(values[i], tmp))
                    return false;
                value_list->push_back(tmp);
            }
            return true;
        }

        virtual std::string to_string()
        {
            std::string result;
            for (size_t i = 0; i < value_list->size(); i++)
            {
                if (i > 0) result += " ";
                result += htio2::to_string((*value_list)[i]);
            }
            return result;
        }

        std::vector<T>* value_list;
    };

public:
    Option(const std::string& name_long,
           char               name_short,
           const std::string& group,
           bool*              value_store,
           int32_t            flags,
           const std::string& option_desc)
        : name_long(name_long)
        , name_short(name_short)
        , group(group)
        , store(new BoolSwitch(value_store))
        , flags(flags)
        , option_desc(option_desc)
        , value_desc("")
    {
    }

    template<typename T>
    Option(const std::string& name_long,
           char               name_short,
           const std::string& group,
           T*                 value_store,
           int32_t            flags,
           const std::string& option_desc,
           const std::string& value_desc)
        : name_long(name_long)
        , name_short(name_short)
        , group(group)
        , store(new SingleValue<T>(value_store))
        , flags(flags)
        , option_desc(option_desc)
        , value_desc(value_desc)
    {
    }

    template<typename T>
    Option(const std::string& name_long,
           char               name_short,
           const std::string& group,
           std::vector<T>*    value_store,
           int32_t            flags,
           ValueLimit         value_num_limit,
           const std::string& option_desc,
           const std::string& value_desc)
        : name_long(name_long)
        , name_short(name_short)
        , group(group)
        , store(new MultiValue<T>(value_store, value_num_limit))
        , flags(flags)
        , option_desc(option_desc)
        , value_desc(value_desc)
    {
    }

    Option(Option&& other) noexcept;

    // copying is not allowed
    Option(const Option& other) = delete;
    Option& operator = (const Option& other) = delete;

    virtual ~Option();

protected:
    static bool validate_name(char name);
    static bool validate_name(const std::string& name);
    std::string format_doc(size_t w_line, size_t w_key_left, size_t w_key, size_t w_key_right) const;
    std::string format_doc_key() const;

protected:
    std::string name_long;
    char        name_short;
    std::string group;
    ValueBase*  store = nullptr;
    uint32_t    flags = 0;
    std::string option_desc;
    std::string value_desc;
};


class OptionParser: public htio2::RefCounted
{
public:
    typedef SmartPtr<OptionParser> Ptr;
    typedef SmartPtr<const OptionParser> ConstPtr;

public:
    bool add_option(Option& option);
    void parse_options(int& argc, char** argv);
    std::string format_document();

protected:
    Option* find_option_long(const std::string& key);
    Option* find_option_short(char key);

protected:
    size_t                          group_id_seed = 0;
    std::map<std::string, size_t>   group_ids;
    std::vector<Option*>            options;
    std::map<char, size_t>          options_by_short;
    std::map<std::string, size_t>   options_by_long;
};

} // namespace htio2

#endif // HTIO2_OPTION_PARSER
