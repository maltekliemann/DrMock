/* Copyright 2019 Ole Kliemann, Malte Kliemann
 *
 * This file is part of DrMock.
 *
 * DrMock is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * DrMock is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with DrMock.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <DrMock/Test.h>
#include "mock/SharedStateMock.h"

using namespace outer::inner;

DRTEST_TEST(success)
{
  {
    SharedStateMock foo{};
    foo.mock.set().state()
        .transition("*", "on", true)
        .transition("*", "off", false);
    foo.mock.get().state()
        .returns("off", false)
        .returns("on", true);
    // Make some random calls.
    foo.set(false);
    foo.get();
    foo.set(true);
    foo.set(true);
    DRTEST_ASSERT(foo.get());
  }

  {
    SharedStateMock foo{};
    foo.mock.set().state()
        .transition("*", "on", true)
        .transition("*", "off", false);
    foo.mock.get().state()
        .returns("off", false)
        .returns("on", true);
    // Make some random calls.
    foo.set(false);
    foo.get();
    foo.set(true);
    foo.set(true);
    foo.set(false);
    DRTEST_ASSERT(not foo.get());
  }
}
