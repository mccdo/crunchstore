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

#include <string>
#include <boost/any.hpp>
#include <Poco/Data/BLOB.h>

namespace Poco
{
namespace Data
{
class Statement;
}    
}

// TODO: cull include deps

/// @file BindableAnyWrapper.h

/// @class BindableAnyWrapper
/// This class provides a means to bind values returned on the stack to a
/// Poco::Data::Statement. Poco::Data::Statement expects a constant reference
/// for the binding. When dealing with the return value of functions

namespace crunchstore
{

class BindableAnyWrapper
{
public:
    BindableAnyWrapper( );
    BindableAnyWrapper( const BindableAnyWrapper& orig );
    virtual ~BindableAnyWrapper( );

    bool BindValue( Poco::Data::Statement* statement, boost::any value );
private:
    bool mBool;
    int mInt;
    float mFloat;
    double mDouble;
    std::string mString;
    Poco::Data::BLOB mBLOB;

};

} // namespace crunchstore
