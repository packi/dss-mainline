/*
    Copyright (c) 2015 digitalSTROM AG, Zurich, Switzerland

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

namespace ds {

/// Poor man's std::nullptr_t replacement.
/// TODO(c++11): refactor to std::nulptr_t in header <cstddef>
struct Nullptr {};
}
