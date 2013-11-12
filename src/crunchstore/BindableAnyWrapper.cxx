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
DIAG_OFF(unused-parameter)
#include <Poco/Version.h>
#if POCO_VERSION > 0x01050000
    #include <Poco/Data/LOBStream.h>
    #define POCO_KEYWORD_NAMESPACE Poco::Data::Keywords::
#else
    #include <Poco/Data/BLOBStream.h>
    #define POCO_KEYWORD_NAMESPACE Poco::Data::
#endif
DIAG_ON(unused-parameter)

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
        (*statement), POCO_KEYWORD_NAMESPACE use( mBool );
        returnValue = true;
    }
    else if( value.type( ) == typeid (int ) )
    {
        mInt = boost::any_cast< int >( value );
        (*statement), POCO_KEYWORD_NAMESPACE use( mInt );
        returnValue = true;
    }
    else if( value.type( ) == typeid (float ) )
    {
        mFloat = boost::any_cast< float >( value );
        (*statement), POCO_KEYWORD_NAMESPACE use( mFloat );
        returnValue = true;
    }
    else if( value.type( ) == typeid (double ) )
    {
        mDouble = boost::any_cast< double >( value );
        (*statement), POCO_KEYWORD_NAMESPACE use( mDouble );
        returnValue = true;
    }
    else if( boost::any_cast< std::string > ( &value ) )
    {
        mString = boost::any_cast< std::string > ( value );
        (*statement), POCO_KEYWORD_NAMESPACE use( mString );
        returnValue = true;
    }
    else if( boost::any_cast< std::vector<char> >( &value ) )
    {
        std::vector<char> data = boost::any_cast< std::vector<char> >( value );
        mBLOB.operator =( data );
        (*statement), POCO_KEYWORD_NAMESPACE use( mBLOB );
        returnValue = true;
    }

    return returnValue;
}
////////////////////////////////////////////////////////////////////////////////
}
