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

#include <string>
#include <boost/any.hpp>

namespace crunchstore
{

class CRUNCHSTORE_EXPORT SearchCriterion
{
public:

    enum OPERATOR{ AND, OR, LIKE, BETWEEN };

    SearchCriterion()
        :
        m_isOperatorCriterion( false )
    {
        ;
    }

    SearchCriterion( OPERATOR )
        :
        m_isOperatorCriterion( true )
    {
        ;
    }

    SearchCriterion( const std::string& key,
                     const std::string& comparison,
                     boost::any value ):
        m_isOperatorCriterion( false ),
        m_key( key ),
        m_comparison( comparison ),
        m_value( value )
    {

    }

    bool m_isOperatorCriterion;
    std::string m_key;
    std::string m_comparison;
    boost::any m_value;
};

} // crunchstore
