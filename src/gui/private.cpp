#include "private.hpp"

#if defined(IMGUI) && defined(GFX_AVAILABLE)

namespace ogm::gui
{
    void ImageComponent::target(geometry::Vector<int32_t> dimensions)
    {
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &m_prev_fbo);
        if (!m_fbo)
        {
            glGenFramebuffers(1, &m_fbo);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

        if (dimensions.x < 1) dimensions.x = 1;
        if (dimensions.y < 1) dimensions.y = 1;
        if (m_texture)
        {
            if (m_dimensions != dimensions)
            {
                // delete texture
                std::cout << "recreating texture." << std::endl;
                glDeleteTextures(1, &m_texture);
                m_texture = 0;
            }
        }

        if (!m_texture)
        {
            m_dimensions = dimensions;
            glGenTextures(1, &m_texture);
            glBindTexture(GL_TEXTURE_2D, m_texture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, dimensions.x, dimensions.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_texture, 0);
        }

        glViewport(0, 0, dimensions.x, dimensions.y);
        glUseProgram(g_ShaderHandle);
    }

    void ImageComponent::button()
    {
        ImGui::ImageButton(
            (void*)(intptr_t)m_texture,
            {
                static_cast<float>(m_dimensions.x),
                static_cast<float>(m_dimensions.y)
            },
            { 0, 0 }, {1, 1}, 0
        );
    }

    void ImageComponent::untarget()
    {
        // TODO: consider binding just previous draw fbo.
        glBindFramebuffer(GL_FRAMEBUFFER, m_prev_fbo);
        m_prev_fbo = 0;
    }

    // from https://github.com/ocornut/imgui/issues/319#issuecomment-147364392
    void DrawSplitter(int split_vertically, float thickness, float* size0, float min_size0, float max_size0)
    {
        ImVec2 backup_pos = ImGui::GetCursorPos();
        if (split_vertically)
            ImGui::SetCursorPosY(backup_pos.y + *size0);
        else
            ImGui::SetCursorPosX(backup_pos.x + *size0);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0,0,0,0));          // We don't draw while active/pressed because as we move the panes the splitter button will be 1 frame late
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f,0.6f,0.6f,0.10f));
        ImGui::Button("##Splitter", ImVec2(!split_vertically ? thickness : -1.0f, split_vertically ? thickness : -1.0f));
        ImGui::PopStyleColor(3);

        ImGui::SetItemAllowOverlap(); // This is to allow having other buttons OVER our splitter.

        if (ImGui::IsItemActive())
        {
            float mouse_delta = split_vertically ? ImGui::GetIO().MouseDelta.y : ImGui::GetIO().MouseDelta.x;

            // Minimum pane size
            if (mouse_delta < min_size0 - *size0)
                mouse_delta = min_size0 - *size0;
            if (mouse_delta > max_size0 - *size0)
                mouse_delta = max_size0 - *size0;

            // Apply resize
            *size0 += mouse_delta;
        }
        else
        {
            if (*size0 > max_size0)
            {
                *size0 = *size0*0.9 + (max_size0) * 0.1 - 1;
            }
            if (*size0 < min_size0)
            {
                *size0 = min_size0;
            }
        }
        ImGui::SetCursorPos(backup_pos);
    }

    geometry::Vector<coord_t> get_mouse_position_from_cursor()
    {
        ImVec2 item_position = ImGui::GetCursorScreenPos();
        return
        {
            ImGui::GetMousePos().x - item_position.x,
            ImGui::GetMousePos().y - item_position.y
        };
    }
}

#endif