// Copyright (C) 2010-2011 von Karman Institute for Fluid Dynamics, Belgium
//
// This software is distributed under the terms of the
// GNU Lesser General Public License version 3 (LGPLv3).
// See doc/lgpl.txt and doc/gpl.txt for the license text.

#ifndef CF_GUI_Client_uTests_CommonFunctions_hpp
#define CF_GUI_Client_uTests_CommonFunctions_hpp

////////////////////////////////////////////////////////////////////////////

#include "UI/Core/NRoot.hpp"

////////////////////////////////////////////////////////////////////////////

namespace CF {
namespace UI {
namespace CoreTest {

  //////////////////////////////////////////////////////////////////////////

  Core::NRoot::Ptr makeTreeFromFile();

  class CommonFunctionsTest : public QObject
  {
    Q_OBJECT

  private slots:

    void test_makeTreeFromFile();

  }; // class CommonFunctionsTest

  //////////////////////////////////////////////////////////////////////////

} // CoreTest
} // UI
} // CF

////////////////////////////////////////////////////////////////////////////

#endif // CF_GUI_Client_uTests_CommonFunctions_hpp
