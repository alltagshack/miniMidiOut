#ifndef _RECYCLE_LIST_HPP
#define _RECYCLE_LIST_HPP 1

#include <list>
#include <memory>
#include <algorithm>
#include <utility>
#include <ranges>

template<typename T>
class RecycleList {
public:
    using container_type = std::list<std::unique_ptr<T>>;
    using iterator = typename container_type::iterator;

    RecycleList() = default;
    ~RecycleList() = default;

    template<typename... Args>
    bool Add (Args&&... args) {
        auto it = std::find_if(
            items.begin(),
            items.end(),
            [](const std::unique_ptr<T>& up) {
                return up->_isDeleted;
            }
        );

        if (it != items.end())
        {
            *it = std::make_unique<T>(std::forward<Args>(args)...);
            (*it)->_isDeleted = false;
            return false;
        }
        else
        {
            items.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
            return true;
        }
    }
    
    bool IsEmpty () const
    {
        return items.empty() || std::all_of(
            items.begin(),
            items.end(),
            [](const std::unique_ptr<T>& up) {
                return up->_isDeleted;
            }
        );
    }

    template<typename Pred>
    auto All (Pred pred) {
        return items | std::views::filter(pred);
    }

    iterator begin() {
        return items.begin();
    }
    
    iterator end() {
        return items.end();
    }

private:
    container_type items;
};

#endif
