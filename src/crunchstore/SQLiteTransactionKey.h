/*************** <auto-copyright.rb BEGIN do not edit this line> **************
 *
 * Copyright 2012-2012 by Ames Laboratory
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 *************** <auto-copyright.rb END do not edit this line> ***************/
#pragma once

#include <crunchstore/ExportConfig.h>
#include <crunchstore/TransactionKey.h>

//This is a hack to get around an issue with POCO 1.5.1 or greater
using namespace std;

#include <crunchstore/CompilerGuards.h>
DIAG_OFF(unused-parameter)
#include <Poco/Data/Session.h>
DIAG_ON(unused-parameter)

namespace crunchstore
{

class CRUNCHSTORE_EXPORT SQLiteTransactionKey: public TransactionKey
{
public:
    SQLiteTransactionKey( Poco::Data::Session& session );
    virtual ~SQLiteTransactionKey();
    virtual std::string GetTypeString() const{ return std::string("SQLite"); }

    void SetSession( Poco::Data::Session& session );
    Poco::Data::Session GetSession() const;
    bool IsSet() const;

private:
    Poco::Data::Session m_session;
    bool m_set;
};

} // namespace
