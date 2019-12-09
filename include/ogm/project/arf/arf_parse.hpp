#pragma once

#include <vector>
#include <string_view>
#include <map>

namespace ogm::project
{
    /*
    # NAME detail1:detail2:... ------------------------

    key1: value1
    key2: value2

    # SUBSEC1 ... -------------------------------------

    # SUBSEC2 ... -------------------------------------

    # SUBSEC3 ... -------------------------------------

    */
    struct ARFSection
    {
    public:
        std::string m_name;
        std::vector<std::string> m_details;
        std::vector<ARFSection*> m_sections;
        std::map<std::string, std::string> m_dict;
        std::vector<std::string> m_list;
        std::string m_text;

    public:
        ~ARFSection()
        {
            for (ARFSection* ogm : m_sections)
            {
                delete ogm;
            }

            m_sections.clear();
        }

	    const std::string& get_value(const std::string& key, const std::string& _default) const
        {
            auto iter = m_dict.find(key);
            if (iter != m_dict.end())
            {
                return iter->second;
            }
            else
            {
                return _default;
            }
        }

        bool has_value(const std::string& key) const
        {
            auto iter = m_dict.find(key);
            return (iter != m_dict.end());
        }
    };

    // describes the hierarchy of an AR format.
    struct ARFSchema
    {
        std::string m_name;
        enum content_type_t
        {
            DICT,
            LIST,
            TEXT
        } m_content_type;

        // schema for subsection
        ARFSchema* m_component_schema = nullptr;

    public:
        ARFSchema(ARFSchema&& other)
            : m_name(std::move(other.m_name))
            , m_content_type(other.m_content_type)
            , m_component_schema(other.m_component_schema)
        {
            other.m_component_schema = nullptr;
        }

        ARFSchema(const std::string& name, content_type_t type, ARFSchema* component = nullptr)
            : m_name(name)
            , m_content_type(type)
            , m_component_schema(component)
        { }

        ARFSchema(const std::string& name, content_type_t type, ARFSchema&& component)
            : m_name(name)
            , m_content_type(type)
            , m_component_schema(new ARFSchema(std::move(component)))
        { }

        ~ARFSchema()
        {
            if (m_component_schema) delete m_component_schema;
        }
    };

    void arf_parse(const ARFSchema*, const char* source, ARFSection&);

    // parses array of the form [a, b, c, ...]
    void arf_parse_array(const char* source, std::vector<std::string_view>& out);
}
