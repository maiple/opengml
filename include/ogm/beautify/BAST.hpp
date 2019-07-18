#pragma once

#include <string>
#include <vector>
#include <map>

namespace ogm
{
namespace beautify
{

// beautifier AST
// this is a language-agnostic tree of output lines, created as an intermediate
// step in beautification.


class BAST
{
public:
    virtual ~BAST() = default;
};

class BASTChunk : public BAST
{
public:
    std::string content;
    
    // some chunks (e.g. those ending with a 1-line comment) must be followed by a line break.
    bool m_force_eol = false;
};

class BASTLine : public BAST
{
public:
    std::vector<BAST*> m_children;
};

class BASTList : public BAST
{
public:
    enum class BraceStyle
    {
        no_braces,
        curly,
        paren,
        square
    } m_style = BraceStyle::no_braces;
    
    enum class LayoutHint
    {
        single_line,
        body
    } m_hint = LayoutHint::single_line;
    
    std::vector<BAST*> m_children;
    
    // force each child on own line.
    bool m_force_lines = false;
};

}
}