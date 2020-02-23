#include "ogm/asset/AssetTable.hpp"
#include "ogm/asset/Image.hpp"

#include "ogm/geometry/Vector.hpp"
#include "ogm/geometry/aabb.hpp"

#include "ogm/common/types.hpp"

#include <functional>

namespace ogm
{
namespace interpreter
{

typedef int32_t tpage_id_t;
typedef int32_t surface_id_t;

struct TexturePage
{
    typedef std::function<asset::Image*()> ImageSupplier;
    uint32_t m_gl_tex = 0;

    // optional (allows drawing to and blitting from)
    uint32_t m_gl_framebuffer = 0;
    
    // optional (depth buffer)
    uint32_t m_gl_tex_depth = 0;

    // permissible to be freed and reloaded from disk later.
    bool m_volatile = true;

    // application surface cannot be deleted
    bool m_no_delete = false;

    // where to find this texture data if it is deleted. (optional)
    ImageSupplier m_callback = []() -> asset::Image* { return nullptr; };

    // not set until data is loaded.
    ogm::geometry::Vector<uint32_t> m_dimensions{ 0, 0 };

    // loads the image from the cache if not already loaded.
    // returns false on failure.
    bool cache();

    ~TexturePage();
};

// points to a region in a texture page
struct TextureView
{
    // which texture page this texture is on.
    TexturePage* m_tpage;

    // uv coordinates, scaled to 0-1.
    ogm::geometry::AABB<real_t> m_uv;

    // converts local normalized uv coordinates to normalized tpage coords.
    inline real_t u_global(real_t u)
    {
        return u * m_uv.width() + m_uv.left();
    }

    inline real_t v_global(real_t v)
    {
        return v * m_uv.height() + m_uv.top();
    }

    inline ogm::geometry::Vector<real_t> uv_global(ogm::geometry::Vector<real_t> uv)
    {
      return {
        u_global(uv.x),
        v_global(uv.y)
      };
    }

    // converts local normalized uv coordinates to tpage integer coords.
    inline uint32_t u_global_i(real_t u)
    {
        m_tpage->cache();
        return u_global(u) * m_tpage->m_dimensions.x;
    }

    inline uint32_t v_global_i(real_t v)
    {
        m_tpage->cache();
        return v_global(v) * m_tpage->m_dimensions.y;
    }

    inline ogm::geometry::Vector<uint32_t> uv_global_i(ogm::geometry::Vector<real_t> uv)
    {
      return {
        u_global_i(uv.x),
        v_global_i(uv.y)
      };
    }

    // integer dimensions on tpage
    inline ogm::geometry::Vector<uint32_t> dim_global_i()
    {
       m_tpage->cache();
        return {
            static_cast<uint32_t>(m_uv.width() * m_tpage->m_dimensions.x),
            static_cast<uint32_t>(m_uv.height() * m_tpage->m_dimensions.y)
        };
    }

    TextureView()
        : m_tpage(nullptr)
        , m_uv{0, 0, 1, 1}
    { }

    TextureView(TexturePage* tp)
        : m_tpage(tp)
        , m_uv{0, 0, 1, 1}
    { }
};

// can retrieve and restore textures from assets on demand
class TextureStore
{
    struct ImageDescriptor
    {
        asset_index_t m_asset_index;
        size_t m_image_index;

        bool operator<(const ImageDescriptor& other) const
        {
            if (m_asset_index == other.m_asset_index)
            {
                return m_image_index < other.m_image_index;
            }
            return m_asset_index < other.m_asset_index;
        }

        ImageDescriptor(const ImageDescriptor& other)
            : m_asset_index(other.m_asset_index)
            , m_image_index(other.m_image_index)
        { }

        ImageDescriptor(asset_index_t index)
            : m_asset_index(index)
            , m_image_index(0)
        { }

        ImageDescriptor(asset_index_t asset_index, size_t image_index)
            : m_asset_index(asset_index)
            , m_image_index(image_index)
        { }
    };

    std::vector<TexturePage*> m_pages;

    // maps assets to texture views.
    // has ownership over these pointers.
    std::map<ImageDescriptor, TextureView*> m_descriptor_map;

    std::vector<std::pair<TexturePage*, TextureView*>> m_surface_map;

public:    
    // gives a callback that supplies an image when it is needed.
    TextureView* bind_asset_to_callback(ImageDescriptor, TexturePage::ImageSupplier);

    // binds an asset by copying an existing texture.
    TextureView* bind_asset_copy_texture(ImageDescriptor, TextureView*, geometry::AABB<uint32_t> from);

    // retrive texture for existing asset
    TextureView* get_texture(ImageDescriptor);
    
    TexturePage* create_tpage_from_callback(TexturePage::ImageSupplier);
    
    TexturePage* create_tpage_from_image(asset::Image&);

    // arranges and blits source images onto a texture page.
    // any sources which were not added will be pushed onto outUVs
    // as 0x0 rectangles.
    // If no sources were added at all, nullptr is returned.
    TexturePage* arrange_tpage(const std::vector<TextureView*>& sources, std::vector<ogm::geometry::AABB<real_t>>& outUVs, bool smart=true);

    // constructs a new textureview for the given page at the given coordinates.
    TextureView* bind_asset_to_tpage_location(ImageDescriptor, TexturePage* tpage, ogm::geometry::AABB<real_t> location = { 0.0, 0.0, 1.0, 1.0 });

    // returns -1 on failure
    surface_id_t create_surface(ogm::geometry::Vector<uint32_t> dimensions);

    TexturePage* get_surface(surface_id_t id)
    {
        if (id >= m_surface_map.size())
        {
            return nullptr;
        }
        return m_surface_map.at(id).first;
    }

    TextureView* get_surface_texture_view(surface_id_t id)
    {
        if (id >= m_surface_map.size())
        {
            return nullptr;
        }
        return m_surface_map.at(id).second;
    }

    uint32_t get_texture_pixel(TexturePage*, ogm::geometry::Vector<uint32_t> position);

    void resize_surface(surface_id_t id, ogm::geometry::Vector<uint32_t> dimensions);
    void free_surface(surface_id_t id);

    // TODO: create actual proper big texture pages and then bind images to those?

    ~TextureStore();
};

}
}
