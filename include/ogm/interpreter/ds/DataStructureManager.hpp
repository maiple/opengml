#pragma once
#include <vector>
#include <cstdlib>

namespace ogm { namespace interpreter
{
    typedef size_t ds_index_t;

    template <class DataStructure>
    class DataStructureManager
    {
    public:
        bool ds_exists(ds_index_t index)
        {
            if (index >= m_datastructures.size())
            {
                return false;
            }

            ogm_assert(m_tombstones.size() == m_datastructures.size());
            ogm_assert(m_tombstones.size() > index);

            return !m_tombstones.at(index);
        }

        inline DataStructure& ds_get(ds_index_t index)
        {
            return *m_datastructures.at(index);
        }

        inline const DataStructure& ds_get(ds_index_t index) const
        {
            return *m_datastructures.at(index);
        }

        template <typename... T>
        inline ds_index_t ds_new(T&&... args)
        {
            ds_index_t index = create_next_index();
            ogm_assert(index < m_tombstones.size());
            m_tombstones.at(index) = false;
            m_datastructures[index] = new DataStructure(std::forward<T>(args)...);
            return index;
        }

        inline void ds_delete(ds_index_t index)
        {
            if (!ds_exists(index))
            {
                return;
            }

            DataStructure*& ds = m_datastructures.at(index);
            delete ds;
            ds = nullptr;
            ogm_assert(index < m_tombstones.size());
            m_tombstones.at(index) = true;
            m_min_free = std::min(m_min_free, index);
        }

        inline void clear()
        {
            for (DataStructure*& ds : m_datastructures)
            {
                delete ds;
                ds = nullptr;
            }
            m_tombstones.clear();
            m_datastructures.clear();
            m_min_free = 0;
        }

        #ifdef OGM_GARBAGE_COLLECTOR
        void gc_integrity_check()
        {
            for (DataStructure* ds : m_datastructures)
            {
                if (ds)
                {
                    ds->gc_integrity_check();
                }
            }
        }
        #endif

    private:
        inline ds_index_t create_next_index()
        {
            for (ds_index_t next = m_min_free; next < m_datastructures.size(); next++)
            {
                ogm_assert(next < m_tombstones.size());
                if (m_tombstones.at(next))
                {
                    m_min_free = next;
                    return next;
                }
            }

            ogm_assert(m_datastructures.size() == m_tombstones.size());

            ds_index_t next = m_datastructures.size();
            m_tombstones.push_back(false);
            m_datastructures.push_back(nullptr);
            m_min_free = next;
            return next;
        }
    private:
        std::vector<bool> m_tombstones;
        std::vector<DataStructure*> m_datastructures;
        // smallest free index.
        size_t m_min_free = 0;
    };
}}
