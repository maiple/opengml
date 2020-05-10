#pragma once

#include "Resource.hpp"

#include "ogm/asset/AssetTable.hpp"
#include "ogm/bytecode/BytecodeTable.hpp"
#include "ogm/ast/parse.h"
#include "ogm/bytecode/bytecode.hpp"

namespace ogm::project {

class ResourceShader : public Resource {
public:
    ResourceShader(const char* path, const char* name);

    void load_file() override;
    void parse(const bytecode::ProjectAccumulator& acc) override;
    void precompile(bytecode::ProjectAccumulator&);
    void compile(bytecode::ProjectAccumulator&, const bytecode::Library* library);
    const char* get_name() { return m_name.c_str(); }
    ~ResourceShader()
    { }

private:
    // data set during initialization
    std::string m_path;

private:
    std::string m_source;
};

}
