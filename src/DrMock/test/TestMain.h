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

#ifndef DRMOCK_SRC_DRMOCK_TEST_TESTMAIN_H
#define DRMOCK_SRC_DRMOCK_TEST_TESTMAIN_H

#ifdef DRTEST_USE_QT
#include <QCoreApplication>
#include <QTimer>
#endif

#include <DrMock/test/FunctionInvoker.h>
#include <DrMock/test/Global.h>
#include <DrMock/utility/ILogger.h>
#include <DrMock/utility/Logger.h>

namespace drtest { namespace detail {

FunctionInvoker initGlobal{[] () { drutility::Singleton<Global>::set(std::make_shared<Global>()); }};

}} // namespaces

int
main(int argc, char** argv)
{
  using ILogger = drutility::ILogger;
  using Logger = drutility::Logger;
  using GlobalSingleton = drutility::Singleton<drtest::detail::Global>;
  using LoggerSingleton = drutility::Singleton<ILogger>;

  LoggerSingleton::set(std::make_shared<Logger>());

#ifdef DRTEST_USE_QT
  QCoreApplication qapp{argc, argv};
  QTimer::singleShot(0, [&] ()
      {
        GlobalSingleton::get()->runTestsAndLog();
        qapp.exit();
      }
    );
  qapp.exec();
#else
  GlobalSingleton::get()->runTestsAndLog();
#endif

  return static_cast<int>(GlobalSingleton::get()->num_failures());
}

#endif /* DRMOCK_SRC_DRMOCK_TEST_TESTMAIN_H */
