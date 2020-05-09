#pragma once

#include "ogm/common/error.hpp"

#include <string>
#include <memory>
#include <vector>

namespace ogm::project {

// identifies a resource in a project.
typedef std::string resource_id_t;
static const resource_id_t k_no_resource_id = "";

class ResourceList;
class ResourceLeaf;

class ResourceTree {
public:
    virtual bool is_leaf() const=0;

    // name of resource or folder
    std::string m_name;
    
    // hide resource when displayed in tree
    bool m_is_hidden = false;

    // for trees --
    virtual size_t get_child_count() const=0;
    virtual ResourceTree* get_child(size_t index)=0;
    virtual const ResourceTree* get_child(size_t index) const=0;
    
    // retrieves a child
    virtual ResourceTree* child(const std::string& name)=0;
    // adds or retrieves existing subdirectory
    virtual ResourceList* subdirectory(const std::string& name)=0;
    // adds a leaf
    virtual ResourceLeaf* leaf(const std::string& name, const resource_id_t&)=0;

    // for leaves --
    //! key for resource's entry in resource table
    virtual const resource_id_t& get_resource_id() const=0;
};

// node in resource tree which represents a folder
class ResourceList : public ResourceTree {
public:
    bool is_leaf() const { return false; };

    // for trees --
    size_t get_child_count() const override                     { return m_list.size(); }
    ResourceTree* get_child(size_t index) override              { return m_list.at(index).get(); }
    const ResourceTree* get_child(size_t index) const override  { return m_list.at(index).get(); }
    
    ResourceTree* child(const std::string& name) override;
    ResourceList* subdirectory(const std::string& name) override;
    ResourceLeaf* leaf(const std::string& name, const resource_id_t&) override;

    const resource_id_t& get_resource_id() const override
    {
        throw MiscError("resource lists do not have resource IDs.");
    }
    
    ResourceList()=default;
    
private:
    std::vector<std::unique_ptr<ResourceTree>> m_list;
};

// node in resource tree which represents an actual resource.
class ResourceLeaf : public ResourceTree {    
public:
    bool is_leaf() const override { return true; };

    size_t get_child_count() const override                     { return 0; }
    ResourceTree* get_child(size_t index) override              { return nullptr; }
    const ResourceTree* get_child(size_t index) const override  { return nullptr; }
    
    ResourceTree* child(const std::string& name) override
    {
        throw MiscError("Leaves cannot have children.");
    }
    ResourceList* subdirectory(const std::string& name) override
    {
        throw MiscError("Leaves cannot have children.");
    }
    ResourceLeaf* leaf(const std::string& name, const resource_id_t&) override
    {
        throw MiscError("Leaves cannot have children.");
    }

    const resource_id_t& get_resource_id() const override       { return m_id; }
    
private:
    friend class ResourceList;
    
    ResourceLeaf(const resource_id_t& id)
        : m_id( id )
    { }
    
    resource_id_t m_id;
};

}