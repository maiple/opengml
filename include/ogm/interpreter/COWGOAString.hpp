#pragma once

#include "ogm/common/util.hpp"
#include <string>
#include <ostream>
#include <string_view>
#include <optional>

namespace ogm::interpreter
{
    // copy-on-write, grow-on-append string
    // like a COW string, but does not necessarily
    // copy when appending to the string or taking a substring.
    // uses string views to achieve this.
    class COWGOAString
    {
        struct Range
        {
            size_t m_start;
            size_t m_end;

            bool valid() const
            {
                return m_start <= m_end;
            }

            Range(size_t start, size_t end)
                : m_start(start)
                , m_end(end)
            {
                ogm_assert(valid());
            }

            Range(const Range& other)=default;

            size_t length() const
            {
                ogm_assert(valid());
                return m_end - m_start;
            }

            void union_with(const Range& other)
            {
                m_start = std::min(m_start, other.m_start);
                m_end = std::max(m_end, other.m_end);
            }

            std::optional<Range> intersect(const Range& other)
            {
                size_t start = std::max(m_start, other.m_start);
                size_t end = std::min(m_end, other.m_end);
                if (start <= end) return Range{start, end};
                return std::nullopt;
            }

            bool contains(const Range& other)
            {
                return m_start <= other.m_start && m_end >= other.m_end;
            }

            void operator += (size_t i)
            {
                m_start += i;
                m_end += i;
            }
        };

        // ownership note: self-deletes if refcount decremented to zero.
        struct COWGOAStringData
        {
            explicit COWGOAStringData(const std::string_view& s)
                : m_str(&s.front(), &s.back() + 1)
                , m_refcount(1)
                , m_shared_range(0, s.length())
            { }

            std::string_view get_view(const Range& r) const
            {
                return std::string_view{ &m_str.at(0) + r.m_start, r.length() };
            }

            void decrement()
            {
                --m_refcount;
                if (m_refcount == 0)
                {
                    delete this;
                }
                else
                {
                    unshare_if_single_ref();
                }
            }

            void increment()
            {
                ++m_refcount;
            }

            // marks the given range as shared.
            void mark_shared(const Range& r)
            {
                ogm_assert(r.m_end <= m_str.size());
                m_shared_range.union_with(r);

                unshare_if_single_ref();
            }

            // grows buffer to include the given range.
            void expand_to(const Range& r)
            {
                if (r.m_end > m_str.size())
                {
                    m_str.resize(r.m_end);
                }
            }

            // releases ownership of the given range
            // has no effect on shared ranges.
            void free_range(const Range& r)
            {
                size_t new_size = std::max(r.m_end, m_shared_range.m_end);
                m_str.resize(new_size);
            }

            std::string_view view(const Range& r) const
            {
                ogm_assert(r.m_end <= m_str.size());
                if (m_str.size() == 0) return "";
                return std::string_view(&m_str.at(r.m_start), r.length());
            }

            char* edit(const Range& r)
            {
                if (r.length() == 0) return nullptr;
                ogm_assert(r.m_end >= 0);
                ogm_assert(range_unshared(r));
                expand_to(r);
                return &m_str.at(r.m_start);
            }

            // returns true if editing the given range would require a copy
            bool edit_requires_copy(const Range& redit, const Range& rowned) const
            {
                if (redit.length() == 0) return false;

                // can append:
                if (redit.m_start >= m_shared_range.m_end
                    && (rowned.m_end > m_shared_range.m_end
                        || rowned.m_end == m_str.size()))
                {
                    return false;
                }

                return true;
            }

            // returns true if the whole range is not being shared
            bool range_unshared(const Range& r) const
            {
                if (single_owner()) return true;
                if (r.m_start >= m_shared_range.m_end) return true;
                if (r.m_end <= m_shared_range.m_start) return true;
                return false;
            }

            // returns true if the whole range is used
            bool range_used(const Range& r) const
            {
                if (r.m_end <= m_str.size()) return true;
                return false;
            }

            // returns true if the whole range is unused
            bool range_unused(const Range& r) const
            {
                if (r.m_start >= m_str.size()) return true;
                return false;
            }

            FORCEINLINE bool single_owner() const
            {
                return m_refcount <= 1;
            }

        private:
            std::vector<char> m_str;
            uint32_t m_refcount;
            Range m_shared_range;

            FORCEINLINE void unshare_if_single_ref()
            {
                if (single_owner())
                {
                    evaporate_shared();
                }
            }

            // removes shared range.
            FORCEINLINE void evaporate_shared()
            {
                m_shared_range = Range(0, 0 );
            }
        };

    private:
        COWGOAStringData* m_s;
        Range m_range;
    public:
        explicit COWGOAString(const std::string_view& s)
            : m_s{ new COWGOAStringData(s) }
            , m_range{ 0, s.length() }
        {
            assert_valid();
        }

        explicit COWGOAString(const COWGOAString& s)
            : m_s{ s.m_s }
            , m_range{ s.m_range }
        {
            s.assert_valid();
            m_s->increment();
            m_s->mark_shared(m_range);
            s.assert_valid();
            assert_valid();
        }

        explicit COWGOAString(COWGOAString&& s)
            : m_s{ s.m_s }
            , m_range{ s.m_range }
        {
            assert_valid();
        }

        void assert_valid() const
        {
            #ifdef CGSTRDEBUG
            // our range does not exceed that of the data's.
            ogm_assert(m_s->range_used(m_range));
            #endif
        }

        void assert_no_null() const
        {
            #ifdef CGSTRDEBUG
            std::string_view sv = m_s->view(m_range);
            for (size_t i = 0; i < sv.size(); ++i)
            {
                ogm_assert(sv[i] != 0);
            }
            #endif
        }

        void cleanup()
        {
            if (m_s) m_s->decrement();
            m_s = nullptr;
        }

        FORCEINLINE ~COWGOAString()
        {
            cleanup();
        }

        std::string_view view() const
        {
            assert_no_null();
            return m_s->view(m_range);
        }

        // range in global coords
        char* edit(Range range)
        {
            assert_valid();
            assert_no_null();
            if (m_s->edit_requires_copy(range, m_range))
            {
                assert_valid();
                // adjust range coords for new coords
                range.m_start -= m_range.m_start;
                range.m_end -= m_range.m_start;
                copy();
                assert_valid();
            }
            assert_no_null();
            assert_valid();
            char* c = m_s->edit(range);
            m_range.union_with(range);
            assert_valid();
            return c;
        }

        // convenience
        bool operator==(const std::string& other) const
        {
            return view() == other;
        }

        bool operator==(const COWGOAString& other) const
        {
            return view() == other.view();
        }

        bool operator!=(const std::string& other) const
        {
            return view() != other;
        }

        bool operator!=(const COWGOAString& other) const
        {
            return view() != other.view();
        }

        friend std::ostream& operator<<( std::ostream &o, const COWGOAString &s ) {
           return o << s.view();
        }

        // only sometimes requires copy.
        COWGOAString& operator += (const std::string_view& _lhs) {
            // we have to copy the string_view here, as it could be a view into
            // a codependent string!
            assert_valid();
            assert_no_null();
            std::string lhs{ _lhs };
            const Range& r = { m_range.m_end, m_range.m_end + lhs.length() };
            memcpy(edit(r), &lhs.front(), lhs.length());
            assert_valid();
            assert_no_null();
            return *this;
        }

        // only sometimes requires copy.
        COWGOAString& operator += (const COWGOAString& lhs) {
            return *this += lhs.view();
        }

        // never requires copy.
        // (range in local coords)
        void shrink(Range r)
        {
            assert_valid();
            r += m_range.m_start;
            ogm_assert(m_range.contains(r));
            m_range = r;
            assert_valid();
        }

        void copy()
        {
            assert_valid();
            if (!m_s->single_owner())
            {
                auto* prev_s = m_s;
                std::string_view vprev = view();
                m_s = new COWGOAStringData{ vprev };
                m_range = {0, vprev.length()};

                // decremnt previous
                prev_s->decrement();
                assert_valid();
            }
        }

        size_t length() const
        {
            return m_range.length();
        }

        size_t size() const
        {
            return length();
        }
    };
}
