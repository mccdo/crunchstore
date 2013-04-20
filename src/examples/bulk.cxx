/*************** <auto-copyright.rb BEGIN do not edit this line> **************
 *
 * VE-Suite is (C) Copyright 1998-2012 by Iowa State University
 *
 * Original Development Team:
 *   - ISU's Thermal Systems Virtual Engineering Group,
 *     Headed by Kenneth Mark Bryden, Ph.D., www.vrac.iastate.edu/~kmbryden
 *   - Reaction Engineering International, www.reaction-eng.com
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
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
 * -----------------------------------------------------------------
 * Date modified: $Date$
 * Version:       $Rev$
 * Author:        $Author$
 * Id:            $Id$
 * -----------------------------------------------------------------
 *
 *************** <auto-copyright.rb END do not edit this line> ***************/
#include <stdlib.h>
#include <iostream>

#include <crunchstore/Persistable.h>
#include <crunchstore/Datum.h>
#include <crunchstore/DataManager.h>
#include <crunchstore/NullBuffer.h>
#include <crunchstore/NullCache.h>
#include <crunchstore/DataAbstractionLayer.h>

#include <crunchstore/SQLiteStore.h>

#include <boost/any.hpp>

int main(int argc, char *argv[])
{

    using namespace std;
    using namespace crunchstore;

    // Set up a datamanager to test persistence
    DataManager manager;
    DataAbstractionLayerPtr cache( new NullCache );
    DataAbstractionLayerPtr buffer( new NullBuffer );
    manager.SetCache( cache );
    manager.SetBuffer( buffer );

    // Add an SQLite store
    SQLiteStorePtr sqstore( new SQLiteStore );
    sqstore->SetStorePath( "/tmp/BulkTest.db" );
    manager.AttachStore( sqstore, Store::WORKINGSTORE_ROLE );

    manager.Drop( "TestType" );

    std::cout << "Begin non-bulk insert..." << std::flush;
    for( int idx = 0; idx < 50; ++idx )
    {
        // Build up a persistable with some useful test types
        Persistable q;
        q.SetTypeName( "TestType" );
        q.AddDatum( "Num", 1234.98735 );
        q.AddDatum( "ABool", true );
        q.AddDatum( "AString", std::string("This is a test") );
        q.AddDatum( "AnInt", 19 );

        std::vector<double> dubs;
        dubs.push_back(1.1);
        dubs.push_back(2.2);
        dubs.push_back(3.141592653587);
        q.AddDatum( "DubVec", dubs );

        manager.Save( q );

        if( (1 + idx) % 10 == 0 )
        {
            std::cout << 1 + idx << "..." << std::flush;
        }
    }
    std::cout << "done." << std::endl << std::flush;



    std::cout << "Begin bulk insert..." << std::flush;
    SQLiteTransactionKey key = sqstore->BeginTransaction();
    std::cout << "Transaction key type is: " << key.GetTypeString() << std::flush;
    for( int idx = 0; idx < 500; ++idx )
    {
        // Build up a persistable with some useful test types
        Persistable q;
        q.SetTypeName( "TestType" );
        q.AddDatum( "Num", 1234.98735 );
        q.AddDatum( "ABool", true );
        q.AddDatum( "AString", std::string("This is a test") );
        q.AddDatum( "AnInt", 19 );

        std::vector<double> dubs;
        dubs.push_back(1.1);
        dubs.push_back(2.2);
        dubs.push_back(3.141592653587);
        q.AddDatum( "DubVec", dubs );

        manager.Save( q, crunchstore::Store::DEFAULT_ROLE, key );

        if( (1 + idx) % 10 == 0 )
        {
            std::cout << 1 + idx << "..." << std::flush;
        }
    }
    sqstore->EndTransaction( key );
    std::cout << "done." << std::endl << std::flush;

    return 0;
}

