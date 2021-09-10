#pragma once

namespace ogm
{

// which language features are enabled?
struct LanguageConfig {
    bool m_enums = true;
    bool m_function_defs = true;
    bool m_constructors = true;
};

}