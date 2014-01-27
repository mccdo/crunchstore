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

#ifndef USE_MONGODB
#include <crunchstore/SQLiteStore.h>
#else
#include <crunchstore/MongoStore.h>
#endif

#include <boost/any.hpp>

int main(int, char **)
{

    using namespace std;
    using namespace crunchstore;

    Persistable p;
    p.AddDatum( "Test", 1.2 );

    // We have two ways of getting at the value, neither of which requires the
    // explicit use of boost::any_cast in the application code -- it now happens
    // behind the scenes:
    double d;
    d = p.GetDatum( "Test" )->extract< double >( );
    d = p.GetDatumValue< double >("Test");

    cout << "Recovering double value: " << d << endl;

    cout << "Attempt to cast incorrectly should throw exception..." << endl << "\t";
    // The templated GetDatumValue<>() and Datum->extract<>() will throw
    // an exception if the cast can't be done. The exception tells us what
    // the two incompatible types are.
    try
    {
        cout << p.GetDatumValue< int >("Test") << endl;
        //cout << p.GetDatum( "Test" )->extract< int >( ) << endl;
    }
    catch( char const* e )
    {
        cout << e << endl;
    }

    // Testing the copy constructor
    cout << "Copy constructor...";
    Persistable o;
    o.AddDatum( "Num", 1234.98735 );
    Persistable c( o );
    if( o.GetDatumValue< double >("Num") != c.GetDatumValue< double >("Num") )
    {
        cout << "fail! Reason: Copied value not equal to original" << endl;
    }
    else
    {
        c.SetDatumValue( "Num", 2.0 );
        if( o.GetDatumValue< double >("Num") != c.GetDatumValue< double >("Num") )
        {
            cout << "pass." << endl;
        }
        else
        {
            cout << "fail! Reason: \"Changed\" copy still equal to original" << endl;
        }
    }

    // Set up a datamanager to test persistence
    DataManager manager;
    DataAbstractionLayerPtr cache( new NullCache );
    DataAbstractionLayerPtr buffer( new NullBuffer );
    manager.SetCache( cache );
    manager.SetBuffer( buffer );

#ifndef USE_MONGODB
    // Add an SQLite store
    //DataAbstractionLayerPtr sqstore( new SQLiteStore );
    //static_cast<SQLiteStore*>(sqstore.get())->SetStorePath( "/tmp/DALTest.db" );
    SQLiteStorePtr sqstore( new SQLiteStore );
    sqstore->SetStorePath( "/tmp/DALTest.db" );
    manager.AttachStore( sqstore, Store::WORKINGSTORE_ROLE );
#else
    // Add a mongoDB store
    DataAbstractionLayerPtr mongostore( new MongoStore );
    static_cast<MongoStore*>(mongostore.get())->SetStorePath("localhost");
    //manager.AttachStore( mongostore, Store::BACKINGSTORE_ROLE );
    manager.AttachStore( mongostore, Store::WORKINGSTORE_ROLE );
#endif

    // Build up a persistable with some useful test types
    Persistable q;
    q.SetTypeName( "TestType" );
    q.AddDatum( "Num", 1234.98735 );
    q.AddDatum( "ABool", true );
    q.AddDatum( "AString", std::string("This is a test") );
    q.AddDatum( "AnInt", 19 );

    std::vector<std::string> strs;
    strs.push_back( "Test One" );
    strs.push_back( "Test Two" );
    strs.push_back( "Test Three" );
    strs.push_back( "Test Four" );
    q.AddDatum( "TestVec", strs );

    std::vector<double> dubs;
    dubs.push_back(1.1);
    dubs.push_back(2.2);
    dubs.push_back(3.141592653587);
    q.AddDatum( "DubVec", dubs );

    std::string bdata( "0123456789 10111213141516171819 20212223242526272829" );
    std::vector<char> blob( bdata.begin(), bdata.end() );
    q.AddDatum( "ABlob", blob );

    manager.Drop( "TestType" );

    manager.Save( q );

    // BLOB problems
    std::vector<char> blob2( 100, 'k' );
    q.SetDatumValue( "ABlob", blob2 );

    std::vector<char> bout2 = q.GetDatumValue<std::vector< char > >("ABlob");
    char* pb2 = &(bout2[0]);
    string bstr2( pb2, bout2.size() );
    cout << "tooter: " << bstr2 << endl;

    manager.Save( q );
    //manager.Save( q, Store::BACKING_ROLE );

    // Set the value of "Num" field to some other value. When we load below, this
    // value will be reset to 1234.98735 if the load was successful. We then
    // print out this value as a test.
    q.SetDatumValue( "Num", 7.77 );
    manager.Load( q );
    cout << "This value should be 1234.98735: " << q.GetDatumValue<double>("Num") << endl;

    // Print out the strings that should be stored in TestVec
    cout << "Strings in TestVec field:" << endl;
    std::vector<std::string> strout = q.GetDatumValue<std::vector< std::string > >("TestVec");
    for( size_t i=0; i < strout.size(); ++i )
    {
        cout << "\t" << strout.at(i) << endl;
    }

    // Print out the doubles that should be stored in DubVec
    cout << "Doubles in DubVec field:" << endl;
    std::vector<double> dout = q.GetDatumValue<std::vector< double > >("DubVec");
    for( size_t i=0; i < dout.size(); ++i )
    {
        cout << "\t" << dout.at(i) << endl;
    }

    // Re-form a string from the chars that should be stored in ABlob
    cout << "Chars in ABlob field, re-string-ized:" << endl;
    std::vector<char> bout = q.GetDatumValue<std::vector< char > >("ABlob");
    char* pb = &(bout[0]);
    string bstr( pb, bout.size() );
    cout << "\t" << bstr << endl;

    // Try out MapReduce functionality
#ifdef USE_MONGODB
    // Drop all records with typename MRIN and MROUT
    manager.Drop( "MRIN" );
    manager.Drop( "MROUT" );

    Persistable one;
    one.SetTypeName( "MRIN" );
    one.AddDatum( "Num", 1 );

    Persistable two;
    two.SetTypeName( "MRIN" );
    two.AddDatum( "Num", 2 );

    Persistable three;
    three.SetTypeName( "MRIN" );
    three.AddDatum( "Num", 5 );

    manager.Save( one );
    manager.Save( two );
    manager.Save( three );

    // Placeholder persistable for output of MapReduce. Creating this first
    // gives us a UUID that can be passed into the Map function.
    Persistable mrout;
    mrout.SetTypeName( "MROUT" );
    mrout.AddDatum( "count", 0.0 );
    mrout.AddDatum( "sum", 0.0 );

    std::string mapF = "function() {emit( \"";
    mapF += mrout.GetUUIDAsString();
    mapF += "\", {count: 1, num: this.Num} );}";
    std::string reduceF = "function(key, values) {var result = {count: 0, sum: 0};values.forEach(function(value) {result.count += value.count;result.sum += value.num;});return result;}";
    static_cast<MongoStore*>(mongostore.get())->MapReduce( "MRIN", mapF,
        reduceF, mongo::BSONObj(), mrout.GetUUIDAsString(), "MROUT" );

    manager.Load( mrout );
    cout << "MapReduce output: count = " << mrout.GetDatumValue<double>("count")
         << ", sum = " << mrout.GetDatumValue<double>("sum") << endl;
#endif

    return 0;
}
