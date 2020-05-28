#pragma once

#include "Resource.hpp"

#include "ogm/asset/AssetTable.hpp"
#include "ogm/bytecode/BytecodeTable.hpp"
#include "ogm/bytecode/bytecode.hpp"
#include "ogm/asset/AssetSound.hpp"

#include <string>
#include <map>

namespace ogm::project {

class ResourceSound : public Resource
{
public:
    ResourceSound(const char* path, const char* name);

    void load_file() override;
    void precompile(bytecode::ProjectAccumulator&);

    asset::AssetSound* m_asset;
    std::string m_data_path;
    std::string m_path;
    real_t m_volume = 1;
    real_t m_pan = 0;
    real_t m_bit_rate = 192;
    real_t m_sample_rate = 44100;
    int32_t m_type = 0;
    uint8_t m_bit_depth = 16;
    int32_t m_effects = 0;
    bool m_preload = true;
    bool m_compressed = false;
    bool m_streamed = false;
    bool m_uncompress_on_load = false;
    const char* get_type_name() override { return "sound"; };

private:
    void load_file_xml();
    void load_file_arf();
    void load_file_json();
};

}
