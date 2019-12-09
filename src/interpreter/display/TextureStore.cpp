#include "ogm/interpreter/display/TextureStore.hpp"
#include "Share.hpp"
#include "rectpack2D/finders_interface.h"

#ifdef GFX_AVAILABLE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image.h>

#ifndef EMSCRIPTEN
    #include <GL/glew.h>
    #include <SDL2/SDL.h>
    #ifdef GFX_TEXT_AVAILABLE
        #include <SDL2/SDL_ttf.h>
        extern unsigned char _binary_Default_Font_ttf[];
        extern unsigned int _binary_Default_Font_ttf_len;
    #endif
#else
    #define GL_GLEXT_PROTOTYPES 1
    #include <SDL.h>
    #include <SDL_image.h>
    #include <SDL_opengles2.h>
#endif

using namespace ogm;

namespace ogm { namespace interpreter {

bool TexturePage::cache()
{
    bool is_loaded = true;
    if (m_gl_tex == 0)
    {
        is_loaded = false;
    }
    else
    {
        // TODO: check that texture has not been freed
    }

    if (!is_loaded)
    {
        if (!m_volatile)
        {
            // no way to load data.
            return false;
        }

        //std::cout << "Caching image \"" + image_path + "\"\n";
        // TODO
        int width, height, channel_count;
        unsigned char* data = stbi_load(m_path.c_str(), &width, &height, &channel_count, 4);

        if (!data)
        {
            std::cout << "  - Failed to cache image \"" + m_path + "\"\n";
            return false;
        }

        m_dimensions = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

        //std::cout << "    loaded image " << width << "x" << height << ", " << channel_count << " channels.\n";
        glGenTextures(1, &m_gl_tex);
        glBindTexture(GL_TEXTURE_2D, m_gl_tex);
        glTexImage2D(
            GL_TEXTURE_2D,
            0, // mipmap level
            GL_RGBA, // texture format
            width, height,
            0,
            (channel_count == 4) ? GL_RGBA : GL_RGB, // source format
            GL_UNSIGNED_BYTE, // source format
            data // image data
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        stbi_image_free(data);

        // TODO: LRU cache free.
        return true;
    }
    else
    {
        return true;
    }
}

TexturePage::~TexturePage()
{
    if (m_gl_framebuffer)
    {
        glDeleteFramebuffers(1, &m_gl_framebuffer);
    }

    if (m_gl_tex)
    {
        glDeleteTextures(1, &m_gl_tex);
    }
}

TextureView* TextureStore::bind_asset_to_path(ImageDescriptor id, std::string path)
{
    // create a texture page and view for the asset.
    TexturePage* page = new TexturePage();
    TextureView* view = new TextureView();

    view->m_uv = { 0.0, 0.0, 1.0, 1.0 };
    view->m_tpage = page;

    page->m_path = path;

    m_pages.push_back(page);
    m_descriptor_map[id] = view;

    return view;
}

namespace
{
    bool attach_framebuffer(uint32_t& framebuffer, uint32_t& tex)
    {
        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex, 0);

        // necessary?
        GLenum drawbuff = GL_COLOR_ATTACHMENT0;
        glDrawBuffers(1, &drawbuff);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            glDeleteFramebuffers(1, &framebuffer);
            framebuffer = 0;
            glBindFramebuffer(GL_FRAMEBUFFER, g_gl_framebuffer);
            return true;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, g_gl_framebuffer);
        return false;
    }

    bool gen_tex_framebuffer(uint32_t& framebuffer, uint32_t& tex, ogm::geometry::Vector<uint32_t> dimensions)
    {
        // framebuffer allows rendering to the surface as a target.
        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

        // texture
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, dimensions.x, dimensions.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex, 0);
        GLenum drawbuff = GL_COLOR_ATTACHMENT0;
        glDrawBuffers(1, &drawbuff);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            glDeleteFramebuffers(1, &framebuffer);
            framebuffer = 0;
            glDeleteTextures(1, &tex);
            tex = 0;
            glBindFramebuffer(GL_FRAMEBUFFER, g_gl_framebuffer);
            return true;
        }

        // clear with white
        glColorMask(true, true, true, true);
        glClearColor(0xff, 0xff, 0xff, 0xff);
        glClear( GL_COLOR_BUFFER_BIT );
        glColorMask(g_cm[0], g_cm[1], g_cm[2], g_cm[3]);

        // return to previous framebuffer.
        glBindFramebuffer(GL_FRAMEBUFFER, g_gl_framebuffer);
        return false;
    }
}

TextureView* TextureStore::bind_asset_copy_texture(ImageDescriptor id, TextureView* tv, geometry::AABB<uint32_t> from)
{
    // create a texture page and view for the asset.
    surface_id_t dst = create_surface(
      from.diagonal()
    );

    TextureView* view = get_surface_texture_view(dst);
    TexturePage* page = view->m_tpage;

    // copy source texture view to new asset
    glBindFramebuffer(GL_READ_FRAMEBUFFER, tv->m_tpage->m_gl_framebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, page->m_gl_framebuffer);
    glBlitFramebuffer(
      tv->u_global_i(from.left()),
      tv->v_global_i(from.top()),
      tv->u_global_i(from.right()),
      tv->v_global_i(from.bottom()),
      view->u_global_i(0),
      view->v_global_i(0),
      view->u_global_i(1),
      view->v_global_i(1),
      GL_COLOR_BUFFER_BIT,
      GL_NEAREST
    );

    // Return to previous framebuffer.
    glBindFramebuffer(GL_FRAMEBUFFER, g_gl_framebuffer);

    // TODO: delete framebuffer for new asset's tpage?
    return view;
}

TextureView* TextureStore::get_texture(ImageDescriptor id)
{
    auto result = m_descriptor_map.find(id);
    if (result == m_descriptor_map.end())
    {
        throw MiscError("No texture found for the given asset");
    }
    else
    {
        return result->second;
    }
}

TexturePage* TextureStore::arrange_tpage(const std::vector<TextureView*>& sources, std::vector<ogm::geometry::AABB<real_t>>& outUVs, bool smart)
{
    const ogm::geometry::Vector<uint32_t> TPAGE_DIM_MAX = { 1024, 1024 };

    // use rectpack2D. Code largely taken from their example.
    const auto runtime_flipping_mode = rectpack2D::flipping_option::DISABLED;
    using spaces_type = rectpack2D::empty_spaces<false, rectpack2D::default_empty_spaces>;
    using rect_type = rectpack2D::output_rect_t<spaces_type>;

    const auto max_side = std::min(TPAGE_DIM_MAX.x, TPAGE_DIM_MAX.y);

    const auto discard_step = (smart)
        ? 1
        : 32;

    auto report_successful = [](rect_type&) {
		return rectpack2D::callback_result::CONTINUE_PACKING;
	};

    auto report_unsuccessful = [](rect_type&) {
		return rectpack2D::callback_result::ABORT_PACKING;
	};

    std::vector<rect_type> rectangles;
    for (TextureView* src : sources)
    {
        // add rectangle for source dimensions
        rectangles.emplace_back(
            rectpack2D::rect_xywh(
                TPAGE_DIM_MAX.x, TPAGE_DIM_MAX.y, src->dim_global_i().x, src->dim_global_i().y
            )
        );

        // ensure source has framebuffer (will blit later)
        if (!src->m_tpage->m_gl_framebuffer)
        {
            if (attach_framebuffer(src->m_tpage->m_gl_framebuffer, src->m_tpage->m_gl_tex))
            {
                return nullptr;
            }
        }
    }

    rectpack2D::rect_wh result_bounds = rectpack2D::find_best_packing<spaces_type>(
		rectangles,
		rectpack2D::make_finder_input(
			max_side,
			discard_step,
			report_successful,
			report_unsuccessful,
			runtime_flipping_mode
		)
	);

    bool total_failure = result_bounds.w == 0 || result_bounds.h == 0;
    bool any_success = false;
    result_bounds.w = power_of_two(result_bounds.w);
    result_bounds.h = power_of_two(result_bounds.h);

    size_t success_count = 0;
    for (const auto& rect : rectangles)
    {
        if (rect.x != TPAGE_DIM_MAX.x && !total_failure)
        {
            outUVs.emplace_back(
                static_cast<real_t>(rect.x) / result_bounds.w,
                static_cast<real_t>(rect.y) / result_bounds.h,
                static_cast<real_t>(rect.x + rect.w) / result_bounds.w,
                static_cast<real_t>(rect.y + rect.h) / result_bounds.h
            );
            any_success = true;
        }
        else
        {
            outUVs.emplace_back(
                0, 0, 0, 0
            );
        }
    }

    if (!any_success || total_failure)
    {
        return nullptr;
    }
    else
    {
        // create texture page and blit sources onto it.
        // create a texture page and view for the asset.
        surface_id_t dst = create_surface(
          {
              static_cast<uint32_t>(result_bounds.w),
              static_cast<uint32_t>(result_bounds.h)
          }
        );

        TexturePage* page = get_surface(dst);

        // copy sources onto dst
        for (size_t i = 0; i < sources.size(); ++i)
        {
            TextureView* source = sources.at(i);
            rect_type& rect = rectangles.at(i);

            // if rectangle was not added, continue
            if (rect.x == TPAGE_DIM_MAX.x) continue;

            // copy source texture view to new asset

            glBindFramebuffer(GL_READ_FRAMEBUFFER, source->m_tpage->m_gl_framebuffer);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, page->m_gl_framebuffer);

            glBlitFramebuffer(
              // source
              source->u_global_i(0),
              source->v_global_i(0),
              source->u_global_i(1),
              source->v_global_i(1),

              // destination
              rect.x,
              rect.y,
              rect.x + rect.w,
              rect.y + rect.h,
              GL_COLOR_BUFFER_BIT,
              GL_NEAREST
            );

            // Return to previous framebuffer.
            glBindFramebuffer(GL_FRAMEBUFFER, g_gl_framebuffer);
        }

        // TODO: delete framebuffer for new tpage?
        return page;
    }
}

TextureView* TextureStore::bind_asset_to_tpage_location(ImageDescriptor id, TexturePage* tpage, ogm::geometry::AABB<real_t> location)
{
    TextureView* view = new TextureView();

    view->m_uv = location;
    view->m_tpage = tpage;

    m_descriptor_map[id] = view;

    return view;
}

TextureStore::~TextureStore()
{
    for (auto pair : m_descriptor_map)
    {
        delete pair.second;
    }

    for (auto pair : m_surface_map)
    {
        delete pair.second;
    }

    for (TexturePage* page : m_pages)
    {
        if (page)
        {
            delete page;
        }
    }
}

surface_id_t TextureStore::create_surface(ogm::geometry::Vector<uint32_t> dimensions)
{
    m_pages.emplace_back(new TexturePage());
    TexturePage& tp = *m_pages.back();
    tp.m_dimensions = dimensions;
    tp.m_volatile = false;

    if (gen_tex_framebuffer(tp.m_gl_framebuffer, tp.m_gl_tex, dimensions))
    {
        m_pages.pop_back();
        return -1;
    }

    TextureView* tv = new TextureView;
    tv->m_tpage = &tp;
    tv->m_uv = {0, 0, 1, 1};
    m_surface_map.push_back({&tp, tv});
    return m_surface_map.size() - 1;
}

void TextureStore::resize_surface(surface_id_t id, ogm::geometry::Vector<uint32_t> dimensions)
{
    if (id >= m_surface_map.size())
    {
        return;
    }
    if (m_surface_map.at(id).first == nullptr)
    {
        return;
    }

    ogm_assert(m_surface_map.at(id).first->m_gl_framebuffer != 0);

    uint32_t fb, tex;
    if (gen_tex_framebuffer(fb, tex, dimensions))
    {
        return;
    }

    TexturePage* tp = m_surface_map.at(id).first;
    if (tp->m_gl_framebuffer)
    {
        glDeleteFramebuffers(1, &tp->m_gl_framebuffer);
    }

    if (tp->m_gl_tex)
    {
        glDeleteTextures(1, &tp->m_gl_tex);
    }

    tp->m_gl_framebuffer = fb;
    tp->m_gl_tex = tex;
    tp->m_dimensions = dimensions;
}

void TextureStore::free_surface(surface_id_t id)
{
    if (id >= m_surface_map.size())
    {
        return;
    }
    if (m_surface_map.at(id).first == nullptr)
    {
        return;
    }
    if (m_surface_map.at(id).first->m_no_delete)
    {
        // can't delete this.
        return;
    }

    ogm_assert(m_surface_map.at(id).first->m_gl_framebuffer != 0);

    // TODO: optimize this lookup
    for (int32_t i = m_pages.size() - 1; i >= 0; --i)
    {
        if (m_pages.at(i) == m_surface_map.at(id).first)
        {
            delete m_surface_map.at(id).first;
            delete m_surface_map.at(id).second;
            m_surface_map[id].first = nullptr;
            m_surface_map[id].second = nullptr;
            m_pages[i] = nullptr;
            break;
        }
    }
    if (id + 1 == m_surface_map.size())
    {
        m_surface_map.pop_back();
    }
}

uint32_t TextureStore::get_texture_pixel(TexturePage* tp, ogm::geometry::Vector<uint32_t> position)
{
    glBindFramebuffer(GL_FRAMEBUFFER, tp->m_gl_framebuffer);
    uint32_t u;
    glReadPixels(position.x, position.y, 1, 1, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, &u);

    glBindFramebuffer(GL_FRAMEBUFFER, g_gl_framebuffer);
    return u;
}

}}
#endif
