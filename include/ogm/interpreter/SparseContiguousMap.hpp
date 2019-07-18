#pragma once

#include "ogm/common/error.hpp"

#include <cassert>

#include <vector>
#include <cstring>
#include <string>

#define SCM_USE_STL

#ifdef SCM_USE_STL
#include <map>

namespace ogmi
{
    template<typename Key, typename Value>
    class SparseContiguousMap
    {
        std::map<Key, Value> map;

    public:
        Value& get(Key key)
        {
            return map[key];
        }

        Value& operator[](Key key)
        {
            return map[key];
        }

        // retrieves value, throwing error if no key.
        const Value& at(Key key) const
        {
            return map.at(key);
        }

        const bool contains(Key key) const
        {
            return map.find(key) != map.end();
        }

        const size_t count() const
        {
            return map.size();
        }

        const Key& get_ith_key(size_t i) const
        {
            auto iter = map.begin();
            std::advance(iter, i);
            return std::get<0>(*iter);
        }

        Key& get_ith_key(size_t i)
        {
            auto iter = map.begin();
            std::advance(iter, i);
            return std::get<0>(*iter);
        }

        const Value& get_ith_value(size_t i) const
        {
            auto iter = map.begin();
            std::advance(iter, i);
            return std::get<1>(*iter);
        }

        Value& get_ith_value(size_t i)
        {
            auto iter = map.begin();
            std::advance(iter, i);
            return std::get<1>(*iter);
        }
    };
}

#else

namespace ogmi
{

    // efficient at inserting/looking up when keys are frequently in contiguous ranges.
    // key must be unsigned integer type.

    // contains a pointer to one buffer which is split into two parts:
    // the "value" buffer and the "region" buffer.
    // the value buffer is a list of values, and the region buffer
    // is a binary tree of {ranges in the value buffer associated with
    // their start and end keys: see the Range struct below.}.

    // - insertion: if contiguous with the last range, append the value and grow
    //   that range by one. Otherwise, start a new range. Possibly trigger regenerate.
    // - find: descend the binary region tree
    // - edit/update: find the existing value in the tree and replace it (no need
    //   to edit the region tree.)
    // - regenerate: combine contiguous regions and grow the buffer.
    template<typename Key, typename Value>
    class SparseContiguousMap
    {
    public:
        SparseContiguousMap(size_t initSize = 64)
        {
            // 16 is a guess at the expected number of regions.
            newBuffer(initSize, initSize / 16);
            m_value_count = 0;
            m_region_count = 1;
            Region& init = region_buffer()[0];
            init.m_start = 0;
            init.m_end = 0;
            init.m_loc = 0;
            init.m_left = -1;
            init.m_right = -1;
            m_region_extensible_index = 0;
            m_region_extensible_limit = -1;
        }

        SparseContiguousMap(const SparseContiguousMap<Key, Value>& other)
        {
            m_buffer_size = other.m_buffer_size;
            m_buffer_start = malloc(other.m_buffer_size);
            m_value_capacity = other.m_value_capacity;
            m_value_count = other.m_value_count;
            m_region_start = static_cast<void*>(static_cast<Value*>(m_buffer_start) + m_value_capacity);
            m_region_capacity = other.m_region_capacity;
            m_region_count = other.m_region_count;
            m_region_extensible_index = other.m_region_extensible_index;
            m_region_extensible_limit = other.m_region_extensible_limit;
            memcpy(m_buffer_start, other.m_buffer_start, m_buffer_size);
        }

        ~SparseContiguousMap()
        {
            free(m_buffer_start);
        }

        // returns a reference to the value associated with the given key,
        // adding it if necessary.
        Value& get(Key key)
        {
            return *this->_find<true>(key);
        }

        Value& operator[](Key key)
        {
            return get(key);
        }

        // retrieves value, throwing error if no key.
        const Value& at(Key key) const
        {
            Value* v = const_cast<SparseContiguousMap<Key, Value>*>(this)->_find<false>(key);
            if (!v)
            {
                throw ItemNotFoundException(key);
            }
            return *v;
        }

        const bool contains(Key key) const
        {
            Value* v = const_cast<SparseContiguousMap<Key, Value>*>(this)->_find<false>(key);
            return !!v;
        }

    private:
        struct Region
        {
            Key m_start;
            Key m_end;
            // index in m_vecs
            size_t m_loc;

            // index in m_regions
            size_t m_left = -1;
            size_t m_right = -1;
        };

        class ItemNotFoundException : public std::exception {
        public:
          ItemNotFoundException(Key id)
            : message(std::string("Item not found: ") + std::to_string(id))
          { }
          virtual const char* what() const noexcept override {
            return message.c_str();
          }
        private:
          std::string message;
        };

        template<bool add>
        Value* _find(Key key)
        {
            if (add && (m_value_count == m_value_capacity || m_region_count == m_region_capacity))
            {
                reserve(m_value_capacity * 4, m_region_capacity * 4);
            }

            Region* region = region_buffer() + m_region_count - 1;
            if (add && key == region->m_end && key < m_region_extensible_limit)
            {
                region->m_end++;
                return value_buffer() + m_value_count++;
            }
            else if (region->m_start > key || region->m_end <= key)
            {
                const Region* prevRegion;
                Key limit = -1;
                region = const_cast<Region*>(findRegion(key, prevRegion, limit));
                if (add && region == nullptr)
                {
                    if (key < prevRegion->m_start)
                    {
                        const_cast<Region*>(prevRegion)->m_left = m_region_count;
                    }
                    else
                    {
                        const_cast<Region*>(prevRegion)->m_right = m_region_count;
                    }
                    Region* region = region_buffer() + m_region_count;
                    m_region_extensible_index = m_region_count;
                    region->m_start = key;
                    region->m_end = key + 1;
                    region->m_loc = m_value_count++;
                    region->m_left = -1;
                    region->m_right = -1;
                    m_region_extensible_limit = limit;
                    m_region_count++;
                    return value_buffer() + region->m_loc;
                }
            }

            if (region)
            {
                return value_buffer() + region->m_loc + (key - region->m_start);
            }
            else
            {
                assert(!add);
                return nullptr;
            }
        }

        const Region* findRegion(Key key, const Region*& prev, Key& limit) const
        {
            return findRegion(key, 0, prev, limit);
        }

        const Region* findRegion(Key key, size_t region_index, const Region*& prev, Key& limit) const
        {
            if (region_index >= m_region_count)
            {
                return nullptr;
            }
            else
            {
                const Region* region = region_buffer() + region_index;
                if (key < region->m_start)
                {
                    limit = region->m_start;
                    prev = region;
                    return findRegion(key, region->m_left, prev, limit);
                }
                else if (key >= region->m_end)
                {
                    prev = region;
                    return findRegion(key, region->m_right, prev, limit);
                }
                return region;
            }
        }

        // creates a new buffer back-end, doesn't destroy nor copy in previous.
        void newBuffer(size_t numValues, size_t numRegions)
        {
            size_t valSize = numValues * sizeof(Value);
            size_t regSize = numRegions * sizeof(Region);

            m_value_capacity = numValues;
            m_region_capacity = numRegions;

            m_buffer_size = valSize + regSize;
            m_buffer_start = malloc(valSize + regSize);
            memset(m_buffer_start, 0, valSize);
            m_region_start = static_cast<char*>(m_buffer_start) + valSize;
        }

        // Constructs a list of regions in the region tree according to their position in the tree.
        // For example, the highest-key region might have the lowest index (0) in the region buffer,
        // in which case as a postcondition, ioStarts.back() == 0.
        void collect_region_indices(std::vector<size_t>& ioStarts, size_t region_index = 0)
        {
            const size_t max = -1;
            if (region_index == max)
            {
                return;
            }
            Region* region = region_buffer() + region_index;

            // in-order traversal.
            collect_region_indices(ioStarts, region->m_left);
            ioStarts.push_back(region_index);
            collect_region_indices(ioStarts, region->m_right);
        }

        size_t rebalance_add_from(Value* buff, Region* buff_region, std::vector<size_t>& from, size_t region_min, size_t region_count, size_t region_insert, size_t mark, size_t& oMarked)
        {
            if (region_insert >= region_count)
            {
                return -1;
            }

            if (region_insert == mark)
            {
                oMarked = m_region_count;
            }

            // copy from to.
            Region* rfrom = buff_region + region_insert;
            Region* rto = region_buffer() + m_region_count++;
            rto->m_start = rfrom->m_start;
            rto->m_end = rfrom->m_end;

            // left-recurse (ensures values are stored in order of their keys after rebalancing.)
            rto->m_left  = rebalance_add_from(buff, buff_region, from, region_min, region_insert, (region_min + region_insert) / 2, mark, oMarked);

            // todo: subsume adjacent regions left and right to decrease fragmentation.
            size_t region_pool_count = 1;

            // copy in values
            rto->m_loc = m_value_count;
            for (size_t i = 0; i < rfrom->m_end - rfrom->m_start; i++)
            {
                void* src = static_cast<void*>(buff + rfrom->m_loc + i);
                void* dst = static_cast<void*>(&value_buffer()[m_value_count++]);
                memcpy(dst, src, sizeof(Value));
            }

            // right-recurse
            rto->m_right = rebalance_add_from(buff, buff_region, from, region_insert + region_pool_count, region_count, (region_count + region_insert + region_pool_count) / 2, mark, oMarked);
            return region_insert;
        }

        void reserve(size_t numValues, size_t numRegions)
        {
            if (numValues + 1 >= m_value_capacity || numRegions + 1 >= m_region_capacity)
            {
                regenerate(numValues + 1, numRegions + 1);
            }
        }

        void regenerate(size_t numValues, size_t numRegions)
        {
            void* buff = m_buffer_start;
            void* buff_region = m_region_start;

            std::vector<size_t> region_indices_in_tree;
            region_indices_in_tree.reserve(m_region_count);
            collect_region_indices(region_indices_in_tree);
            size_t prev_region_count = m_region_count;

            newBuffer(numValues, numRegions);

            m_value_count = 0;
            m_region_count = 0;

            rebalance_add_from(static_cast<Value*>(buff), static_cast<Region*>(buff_region), region_indices_in_tree, 0, prev_region_count, prev_region_count / 2, m_region_extensible_index, m_region_extensible_index);

            free(buff);
        }

        inline Value* value_buffer() { return static_cast<Value*>(m_buffer_start); }
        inline const Value* value_buffer() const { return static_cast<const Value*>(m_buffer_start); }
        inline Region* region_buffer() { return static_cast<Region*>(m_region_start); }
        inline const Region* region_buffer() const { return static_cast<const Region*>(m_region_start); }
        inline Region* extensible_region() { region_buffer() + m_region_extensible_index; }

        void* m_buffer_start;
        void* m_region_start;
        size_t m_buffer_size;
        size_t m_value_capacity;
        size_t m_value_count;
        size_t m_region_capacity;
        size_t m_region_count;
        size_t m_region_extensible_index;
        // highest key that the extensible region can potentially add (-1 if infinite)
        Key m_region_extensible_limit;
    };
}

#endif
