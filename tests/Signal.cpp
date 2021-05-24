/* Copyright 2020 Ole Kliemann, Malte Kliemann
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

#define DRTEST_USE_QT
#define DRMOCK_USE_QT  // This is usually set by the mocker.
#include "test/Test.h"
#include "mock/Signal.h"
#include "Dummy.h"

using namespace drmock;

DRTEST_TEST(invokeNoParametersDirectConnection)
{
  Dummy dummy{Qt::DirectConnection};
  Signal<Dummy> signal{&Dummy::no_params};
  signal.invoke(&dummy);
  DRTEST_ASSERT_EQ(dummy.no_params_count(), 1);
}

DRTEST_TEST(invokeNoParametersQueuedConnection)
{
  Dummy dummy{Qt::QueuedConnection};
  Signal<Dummy> signal{&Dummy::no_params};
  signal.invoke(&dummy);
  QEventLoop event_loop{};
  event_loop.processEvents();
  DRTEST_ASSERT_EQ(dummy.no_params_count(), 1);
}

DRTEST_TEST(invokeWithParametersDirectConnection)
{
  Dummy dummy{Qt::DirectConnection};
  auto n = 3;
  QString str{"foo"};
  Signal<Dummy, int, const QString&> signal{
      &Dummy::multiple_params,
      n, str
    };
  signal.invoke(&dummy);
  QEventLoop event_loop{};
  event_loop.processEvents();
  auto [num, ptr] = dummy.multiple_params_value();
  DRTEST_ASSERT_EQ(num, n);
  // Expect no copy!
  DRTEST_ASSERT(ptr);
  DRTEST_ASSERT_EQ(ptr, &str);
}

DRTEST_TEST(invokeWithParametersQueuedConnection)
{
  Dummy dummy{Qt::QueuedConnection};
  auto n = 3;
  QString str{"foo"};
  Signal<Dummy, int, const QString&> signal{
      &Dummy::multiple_params,
      n, str
    };
  signal.invoke(&dummy);
  auto [num, ptr] = dummy.multiple_params_value();
  DRTEST_ASSERT_EQ(num, n);
  // Expect copy!
  DRTEST_ASSERT(ptr);
  DRTEST_ASSERT_NE(ptr, &str);
  DRTEST_ASSERT_EQ(*ptr, str);
}

DRTEST_TEST(invokeWithReferenceDirectConnection)
{
  Dummy dummy{Qt::DirectConnection};
  QString foo = "bar";
  Signal<Dummy, QString&> signal{&Dummy::pass_by_ref, foo};
  signal.invoke(&dummy);
  auto ptr = dummy.pass_by_ref_value();
  DRTEST_ASSERT_EQ(ptr, &foo);
}
