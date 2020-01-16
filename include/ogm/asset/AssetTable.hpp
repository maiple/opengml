#pragma once

#include "Asset.hpp"

//FIXME: remove these imports:
#include "AssetBackground.hpp"
#include "AssetObject.hpp"
#include "AssetSound.hpp"
#include "AssetScript.hpp"
#include "AssetSprite.hpp"
#include "AssetRoom.hpp"
#include "AssetShader.hpp"
#include "AssetFont.hpp"

#include <type_traits>

#include "ogm/common/parallel.hpp"

namespace ogm { namespace asset {

// note: constness and mutices applies only to the asset index.
// a non-const pointer to an asset can be attained from a
// const AssetTable.
class AssetTable
{
public:
    template<typename AssetType=Asset*>
    AssetType get_asset(asset_index_t id) const
    {
        if (id < m_index_asset.size())
        {
            return dynamic_cast<AssetType>(m_index_asset.at(id));
        }
        return nullptr;
    }

    Asset* get_asset(const char* assetName, asset_index_t& outAssetIndex) const
    {
        if (m_name_index.find(assetName) != m_name_index.end())
        {
            outAssetIndex = m_name_index.at(assetName);
            return m_index_asset.at(outAssetIndex);
        }
        outAssetIndex = k_no_asset;
        return nullptr;
    }

    Asset* get_asset(const char* assetName) const
    {
        if (m_name_index.find(assetName) != m_name_index.end())
        {
            asset_index_t index = m_name_index.at(assetName);
            return m_index_asset.at(index);
        }
        return nullptr;
    }

    const char* get_asset_name(asset_index_t id)
    {
        if (id < m_index_name.size())
        {
            return m_index_name.at(id).c_str();
        }
        return nullptr;
    }

    template<typename Asset>
    Asset* add_asset(const char* assetName, asset_index_t* outAssetIndex = nullptr)
    {
        if (outAssetIndex)
        {
            *outAssetIndex = m_index_asset.size();
        }
        m_name_index[assetName] = m_index_asset.size();
        m_index_name.emplace_back(assetName);
        Asset* asset = new Asset();
        m_index_asset.emplace_back(asset);
        return asset;
    }

    asset_index_t asset_count() const
    {
        return m_index_asset.size();
    }

    void clear()
    {
        for (Asset* a : m_index_asset)
        {
            delete a;
        }
        m_name_index.clear();
        m_index_name.clear();
        m_index_asset.clear();
    }

private:
    std::map<std::string, asset_index_t> m_name_index;
    std::vector<std::string> m_index_name;
    std::vector<Asset*> m_index_asset;

public:
    // caller should take out a read lock on the mutex when using this.
    const decltype(m_index_asset) get_asset_vector_NOMUTEX() const
    {
        return m_index_asset;
    }
};

extern const AssetTable emptyAssetTable;

}}
