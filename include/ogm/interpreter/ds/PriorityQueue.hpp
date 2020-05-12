#pragma once

#include "Compare.hpp"

#include "ogm/interpreter/Variable.hpp"

#include <vector>
#include <deque>

namespace ogm::interpreter
{
    struct DSPriorityQueue
    {
        typedef std::pair<Variable, std::deque<Variable>> chained_priority_elt;
        
        static bool comp(const chained_priority_elt& a, const chained_priority_elt& b)
        {
            static DSComparator c;
            return c(a.first, b.first);
        };
        
        // TODO: instead of Deque, use an AVL tree.
        std::deque<chained_priority_elt> m_data;
        
        #ifdef OGM_GARBAGE_COLLECTOR
        void ds_integrity_check()
        {
            for (auto& entry : m_data)
            {
                entry.first.gc_integrity_check();
                for (Variable& v : entry.second)
                {
                    v.gc_integrity_check();
                }
            }
        }
        #endif
        
        // finds the entry for the given priority, adding it if it does not exist.
        decltype(m_data.begin()) get_entry(const Variable& priority)
        {
            chained_priority_elt elt;
            elt.first.copy(priority);
            auto iter = std::lower_bound(m_data.begin(), m_data.end(), elt, comp);
            
            if (iter != m_data.end() && iter->first == priority)
            {
                elt.first.cleanup();
                return iter;
            }
            return m_data.emplace(iter, std::move(elt));
        }
        
        // finds entry by priority, returning m_data.end() if not found.
        decltype(m_data.begin()) find_entry(const Variable& priority)
        {
            chained_priority_elt elt;
            elt.first.copy(priority);
            auto iter = std::lower_bound(m_data.begin(), m_data.end(), elt, comp);
            elt.first.cleanup();
            
            if (iter != m_data.end() && iter->first == priority)
            {
                return iter;
            }
            else
            {
                return m_data.end();
            }
        }
        
        void emplace(const Variable& priority, Variable&& elt)
        {
            auto iter = get_entry(priority);
            iter->second.emplace_back(std::move(elt));
        }
        
        inline bool empty() const
        {
            return m_data.empty();
        }
        
        template<bool min=true>
        Variable remove();
        
        template<bool min=true>
        const Variable& peek() const;
    };
}
