#include "ogm/project/ResourceTree.hpp"

namespace ogm::project {

ResourceTree* ResourceList::child(const std::string& name)
{
    for (size_t i = 0; i < get_child_count(); ++i)
    {
        if (get_child(i)->m_name == name)
        {
            return get_child(i);
        }
    }
    
    return nullptr;
}

ResourceList* ResourceList::subdirectory(const std::string& name)
{
    if (ResourceTree* prev = child(name))
    {
        if (prev->is_leaf())
        {
            throw MiscError("resource tree node already exists and is a leaf.");
        }
        else
        {
            return static_cast<ResourceList*>(prev);
        }
    }
    ResourceList* rl = new ResourceList();
    m_list.emplace_back(rl);
    return rl;
}

ResourceLeaf* ResourceList::leaf(const std::string& name, const resource_id_t& id)
{
    if (ResourceTree* prev = child(name))
    {
        if (!prev->is_leaf())
        {
            throw MiscError("resource tree node already exists and is not a leaf.");
        }
        else
        {
            if (prev->get_resource_id() == id)
            {
                return static_cast<ResourceLeaf*>(prev);
            }
            else
            {
                throw MiscError("resource leaf node already exists with a different id.");
            }
        }
    }
    ResourceLeaf* rl = new ResourceLeaf(id);
    m_list.emplace_back(rl);
    return rl;
}

}