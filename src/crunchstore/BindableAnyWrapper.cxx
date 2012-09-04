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
#include <crunchstore/BindableAnyWrapper.h>

#include <Poco/Data/Binding.h>
#include <Poco/Data/Statement.h>
#include <Poco/Data/BLOBStream.h>

#include <iostream>

namespace crunchstore
{
////////////////////////////////////////////////////////////////////////////////
BindableAnyWrapper::BindableAnyWrapper()
{
}
////////////////////////////////////////////////////////////////////////////////
BindableAnyWrapper::BindableAnyWrapper( const BindableAnyWrapper& )
{
}
////////////////////////////////////////////////////////////////////////////////
BindableAnyWrapper::~BindableAnyWrapper()
{
}
////////////////////////////////////////////////////////////////////////////////
bool BindableAnyWrapper::BindValue( Poco::Data::Statement* statement,
                                   boost::any value )
{
    bool returnValue = false;

    if( value.type( ) == typeid ( bool ) )
    {
        mBool = boost::any_cast< bool >( value );
        (*statement), Poco::Data::use( mBool );
        returnValue = true;
    }
    else if( value.type( ) == typeid (int ) )
    {
        mInt = boost::any_cast< int >( value );
        (*statement), Poco::Data::use( mInt );
        returnValue = true;
    }
    else if( value.type( ) == typeid (float ) )
    {
        mFloat = boost::any_cast< float >( value );
        (*statement), Poco::Data::use( mFloat );
        returnValue = true;
    }
    else if( value.type( ) == typeid (double ) )
    {
        mDouble = boost::any_cast< double >( value );
        (*statement), Poco::Data::use( mDouble );
        returnValue = true;
    }
    else if( boost::any_cast< std::string > ( &value ) )
    {
        mString = boost::any_cast< std::string > ( value );
        (*statement), Poco::Data::use( mString );
        returnValue = true;
    }
    else if( boost::any_cast< std::vector<char> >( &value ) )
    {
        std::vector<char> data = boost::any_cast< std::vector<char> >( value );
        mBLOB.operator =( data );
        (*statement), Poco::Data::use( mBLOB );
        returnValue = true;
    }

    return returnValue;
}
////////////////////////////////////////////////////////////////////////////////
}
