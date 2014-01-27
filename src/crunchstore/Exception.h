#pragma once

#include <stdexcept>

namespace crunchstore
{

class Exception : public std::runtime_error
{
public:

    Exception( const std::string& name, const std::string& message )
        :
          std::runtime_error( name ),
          m_name( name ),
          m_message( message )
    {
    }

    virtual ~Exception() throw()
    {}

    const std::string& name() const
    {
        return m_name;
    }

    const std::string& message() const
    {
        return m_message;
    }

private:
    const std::string& m_name;
    const std::string& m_message;
};

}
