/*
    Copyright (c) 2016 digitalSTROM AG, Zurich, Switzerland

    This file is part of vdSM.

    vdSM is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 2 of the License.

    vdSM is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with vdSM. If not, see <http://www.gnu.org/licenses/>.
*/
#pragma once

#include <ds/common.h>
#include <boost/variant.hpp>
#include <string>
#include <type_traits>
#include <vector>

namespace ds {
namespace collection {

/// Collection Entry stores templated value and is identified by string id.
template <typename T>
struct Entry {
    std::string id;
    T value;

    // TODO(c++11): use DS_MOVABLE_AND_EXPLICIT_COPYABLE
    // Gcc 4.5 does not generate default move assignment operator
    // necessary for e.g. std::vector::erase.
    // We have to provide hand written move assignment operator.
    Entry(Entry&&) = default;
    Entry& operator=(Entry&& x) {
        id = std::move(x.id);
        value = std::move(x.value);
        return *this;
    }
    Entry(std::string id, T value) : id(std::move(id)), value(std::move(value)) {}
};

/// Item was added to collection
template <typename T>
struct Added : public Entry<T> {
    // TODO(c++11): see Entry<T>
    Added(Added&&) = default;
    Added& operator=(Added&& x) {
        static_cast<Entry<T>&>(*this) = std::move(static_cast<Entry<T>&>(*this));
        return *this;
    }
    Added(std::string id, T value) : Entry<T>(std::move(id), std::move(value)) {}
    explicit Added(Entry<T> entry) : Entry<T>(std::move(entry)) {}
};

/// Item was removed from collection
template <typename T>
struct Removed {
    std::string id;

    // TODO(c++11): see Entry<T>
    Removed(Removed&&) = default;
    Removed& operator=(Removed&& x) { id = std::move(x.id); }
    Removed(std::string id) : id(std::move(id)) {}
};

/// Item value in collection was changed.
template <typename T>
struct Changed : public Entry<T> {
    // TODO(c++11): see Entry<T>
    Changed(Changed&&) = default;
    Changed& operator=(Changed&& x) {
        static_cast<Entry<T>&>(*this) = std::move(static_cast<Entry<T>&>(*this));
        return *this;
    }
    Changed(std::string id, T value) : Entry<T>(std::move(id), std::move(value)) {}
    explicit Changed(Entry<T> entry) : Entry<T>(std::move(entry)) {}
};

/// Initial batch of events is over.
struct AllForNow {};

/// Collection content is reset to empty collection.
/// Initial batch will follow ending with `AllForNow`.
struct Reset {};

/// Collection event. Either added, removed, change, all for now or reset.
template <typename T>
struct Event {
    typedef boost::variant<Added<T>, Removed<T>, Changed<T>, AllForNow, Reset> Type;
    // TODO(c++11): drop `::Type` with `using Event = boost::variant<Added<T>..>`
};

/// Map event value by given function.
///
/// TODO(someday): T is not deduced. Can it be deduced?
template <typename T, typename Func>
typename Event<typename std::result_of<Func(T)>::type>::Type mapValue(typename Event<T>::Type event, const Func& func) {
    typedef typename std::result_of<Func(T)>::type T2;
    if (auto added = boost::get<Added<T>>(&event)) {
        return Added<T2>(std::move(added->id), func(std::move(added->value)));
    } else if (auto removed = boost::get<ds::collection::Removed<T>>(&event)) {
        return Removed<T2>(std::move(removed->id));
    } else if (auto changed = boost::get<ds::collection::Changed<T>>(&event)) {
        return Changed<T2>(std::move(changed->id), func(std::move(changed->value)));
    } else if (boost::get<AllForNow>(&event)) {
        return AllForNow();
    } else if (boost::get<Reset>(&event)) {
        return Reset();
    }
    DS_FAIL_IREQUIRE("Unhandled event");
}

/// Local collection of entries stored inside std::vector.
/// Preserves order, provides linear lookup.
template <typename T>
class Vector : public std::vector<Entry<T>> {
public:
    typedef typename std::vector<Entry<T>> Super;

    Vector() : m_isAllForNow(false) {}

    typename Super::const_iterator tryFind(const std::string& id) const {
        for (auto it = Super::begin(); it != Super::end(); ++it) {
            if (it->id == id) {
                return it;
            }
        }
        return Super::end();
    }

    typename Super::iterator tryFind(const std::string& id) {
        for (auto it = Super::begin(); it != Super::end(); ++it) {
            if (it->id == id) {
                return it;
            }
        }
        return Super::end();
    }

    typename Super::iterator find(const std::string& id) const {
        auto it = tryFind(id);
        DS_IREQUIRE(it != Super::end(), std::string("Entry does not exist:") + id);
        return it;
    }

    typename Super::iterator find(const std::string& id) {
        auto it = tryFind(id);
        DS_IREQUIRE(it != Super::end(), std::string("Entry does not exist:") + id);
        return it;
    }

    const T* tryAt(const std::string& id) const {
        auto it = tryFind(id);
        if (it != Super::end()) {
            return &(it->value);
        } else {
            return DS_NULLPTR;
        }
    }

    T* tryAt(const std::string& id) {
        auto it = tryFind(id);
        if (it != Super::end()) {
            return &(it->value);
        } else {
            return DS_NULLPTR;
        }
    }

    const T& at(const std::string& id) const {
        if (auto x = tryAt(id)) {
            return *x;
        }
        DS_FAIL_IREQUIRE(std::string("Key not found: ") + id);
    }

    T& at(const std::string& id) {
        if (auto x = tryAt(id)) {
            return *x;
        }
        DS_FAIL_IREQUIRE(std::string("Key not found: ") + id);
    }

    bool isAllForNow() const { return m_isAllForNow; }

    void add(std::string id, T value) {
        DS_IREQUIRE(tryFind(id) == Super::end(), std::string("Adding duplicate id:" + id));
        Super::emplace_back(Entry<T>(std::move(id), std::move(value)));
    }

    void remove(const std::string& id) {
        auto it = tryFind(id);
        DS_IREQUIRE(it != Super::end(), "Removing non existing id");
        Super::erase(it);
    }

    void change(const std::string& id, T value) { at(id) = std::move(value); }

    void setAllForNow() {
        DS_IREQUIRE(!m_isAllForNow, "AllForNow already set.");
        m_isAllForNow = true;
    }

    // Clears content, clears isAllForNow flag..
    void reset() {
        m_isAllForNow = false;
        Super::clear();
    }

    void applyEvent(typename Event<T>::Type event) {
        if (auto added = boost::get<ds::collection::Added<T>>(&event)) {
            add(std::move(added->id), std::move(added->value));
        } else if (auto removed = boost::get<ds::collection::Removed<T>>(&event)) {
            remove(std::move(removed->id));
        } else if (auto changed = boost::get<ds::collection::Changed<T>>(&event)) {
            change(std::move(changed->id), std::move(changed->value));
        } else if (boost::get<ds::collection::AllForNow>(&event)) {
            setAllForNow();
        } else if (boost::get<ds::collection::Reset>(&event)) {
            reset();
        } else {
            DS_FAIL_IREQUIRE("Unhandled event");
        }
    }

private:
    bool m_isAllForNow;
};

} // namespace collection
} // namespace ds
