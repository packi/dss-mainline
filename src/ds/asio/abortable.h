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
#include <boost/intrusive_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <memory>

namespace ds {
namespace asio {

/// Provides an easy way to track aborted handler calls.
///
/// All handlers in asio are posted to io service queue
/// and invoked from ioService.run() family methods.
/// Once the handler is posted e.g. by timer,
/// it will be exeted even when the timer is cancelled
/// before handler execution.
///
/// This class allows handler to detect that it was aborted.
///
/// ````
/// auto abortableHandle = m_abortable.nextHandle();
/// async_wait([f, abortableHandle](boost::system::error_code e) mutable {
///     if (abortableHandle.isAborted() || e == boost::asio::error::operation_aborted) {
///        return;
///     }
///     ....
/// });
/// ````
class Abortable {
public:
    class Shared;

    Abortable();
    ~Abortable();

    /// Abort currect handler.
    void abort();

    /// Handle hold by currect handler.
    class Handle {
    public:
        /// Is this handle already aborted?
        bool isAborted() const;

        /// TODO(c++14): make move only. But we need C++14 move to capture to make it practical.
        /// We could refactor to `std::unique_ptr<bool>`
        /// with move only semantics and reduce memory overhead with no active handle.
        DS_MOVABLE_AND_EXPLICIT_COPYABLE(Handle);

    private:
        boost::intrusive_ptr<Shared> m_shared;
        unsigned m_seq;
        Handle(boost::intrusive_ptr<Shared> shared, unsigned seq) : m_shared(std::move(shared)), m_seq(seq) {}
        friend class Abortable;
    };

    /// Abort currect handler and return handle for new one.
    Handle nextHandle();

private:
    boost::intrusive_ptr<Shared> m_shared;
};

// Implementation details

class Abortable::Shared : boost::noncopyable {
    mutable unsigned m_refs;
    unsigned m_seq;
    Shared() : m_refs(), m_seq() {}

    friend class Abortable;
    friend class Handle;
    friend void intrusive_ptr_add_ref(const Abortable::Shared*);
    friend void intrusive_ptr_release(const Abortable::Shared*);
};

inline Abortable::Abortable() : m_shared(new Shared()){};
inline Abortable::~Abortable() {
    abort();
}

inline void Abortable::abort() {
    ++m_shared->m_seq;
}

inline bool Abortable::Handle::isAborted() const {
    return m_seq != m_shared->m_seq;
}

inline Abortable::Handle Abortable::nextHandle() {
    abort();
    return Handle(m_shared, m_shared->m_seq);
}

inline void intrusive_ptr_add_ref(const Abortable::Shared* p) {
    ++p->m_refs;
}

inline void intrusive_ptr_release(const Abortable::Shared* p) {
    if (--(p->m_refs) == 0u)
        delete p;
}

} // namespace asio
} // namespace ds
