﻿#pragma once

#include <complex>

#include "cVector2.h"
#include "cVector3.h"

namespace df
{
    template< typename T >
    class cVector4 final
    {
    public:
        cVector4();
        explicit cVector4( const T& _value );
        cVector4( const T& _x, const T& _y, const T& _z, const T& _w );
        cVector4( const cVector4& _other );
        cVector4( const cVector3< T >& _other, const T& _w );
        cVector4( const cVector2< T >& _other, const T& _z, const T& _w );

        cVector4( cVector4&& ) = default;
        ~cVector4()            = default;

        cVector4& operator=( const T& _value );
        cVector4& operator=( const cVector4& _other );
        cVector4& operator=( cVector4&& ) = default;

        bool operator==( const cVector4& _other ) { return x() == _other.x() && y() == _other.y() && z() == _other.z() && w() == _other.w(); }
        bool operator!=( const cVector4& _other ) { return x() != _other.x() || y() != _other.y() || z() != _other.z() || w() != _other.w(); }

        cVector4  operator+( const T& _value ) { return cVector4( x() + _value, y() + _value, z() + _value, w() + _value ); }
        cVector4  operator+( const cVector4& _other ) { return cVector4( x() + _other.x(), y() + _other.y(), z() + _other.z(), w() + _other.w() ); }
        cVector4& operator+=( const T& _value );
        cVector4& operator+=( const cVector4& _other );

        cVector4  operator-() { return cVector4( -x(), -y(), -z(), -w() ); }
        cVector4  operator-( const T& _value ) { return cVector4( x() - _value, y() - _value, z() - _value, w() - _value ); }
        cVector4  operator-( const cVector4& _other ) { return cVector4( x() - _other.x(), y() - _other.y(), z() - _other.z(), w() - _other.w() ); }
        cVector4& operator-=( const T& _value );
        cVector4& operator-=( const cVector4& _other );

        cVector4  operator*( const T& _value ) { return cVector4( x() * _value, y() * _value, z() * _value, w() * _value ); }
        cVector4  operator*( const cVector4& _other ) { return cVector4( x() * _other.x(), y() * _other.y(), z() * _other.z(), w() * _other.w() ); }
        cVector4& operator*=( const T& _value );
        cVector4& operator*=( const cVector4& _other );

        cVector4  operator/( const T& _value ) { return cVector4( x() / _value, y() / _value, z() / _value, w() / _value ); }
        cVector4  operator/( const cVector4& _other ) { return cVector4( x() / _other.x(), y() / _other.y(), z() / _other.z(), w() / _other.w() ); }
        cVector4& operator/=( const T& _value );
        cVector4& operator/=( const cVector4& _other );

        T&       operator[]( const size_t _index ) { return m_array[ _index ]; }
        const T& operator[]( const size_t _index ) const { return m_array[ _index ]; }

        T length() { return std::sqrt( std::pow( x(), 2 ) + std::pow( y(), 2 ) + std::pow( z(), 2 ) + std::pow( w(), 2 ) ); }
        T squaredLength() { return std::pow( x(), 2 ) + std::pow( y(), 2 ) + std::pow( z(), 2 ) + std::pow( w(), 2 ); }

        cVector4& normalize();
        cVector4  normalized();

        T dot( const cVector4& _other ) { return x() * _other.x() + y() * _other.y() + z() * _other.z() + w() * _other.w(); }
        T dot( const cVector4* _other ) { return x() * _other->x() + y() * _other->y() + z() * _other->z() + w() * _other->w(); }

        T angleBetween( const cVector4& _other ) { return acos( dot( _other ) / ( length() * _other.length() ) ); }
        T angleBetween( const cVector4* _other ) { return acos( dot( _other ) / ( length() * _other->length() ) ); }
        T angleBetweenNormalized( const cVector4& _other ) { return acos( dot( _other ) ); }
        T angleBetweenNormalized( const cVector4* _other ) { return acos( dot( _other ) ); }

        cVector4 project( const cVector4& _other ) { return _other * ( dot( _other ) / _other.dot( this ) ); }
        cVector4 project( const cVector4* _other ) { return _other * ( dot( _other ) / _other->dot( this ) ); }

        T&       x() { return m_array[ 0 ]; }
        const T& x() const { return m_array[ 0 ]; }
        T&       y() { return m_array[ 1 ]; }
        const T& y() const { return m_array[ 1 ]; }
        T&       z() { return m_array[ 2 ]; }
        const T& z() const { return m_array[ 2 ]; }
        T&       w() { return m_array[ 3 ]; }
        const T& w() const { return m_array[ 3 ]; }

    private:
        T m_array[ 4 ];
    };

    template< typename T >
    cVector4< T >::cVector4()
    : m_array{ 0, 0, 0, 0 }
    {}

    template< typename T >
    cVector4< T >::cVector4( const T& _value )
    : m_array{ _value, _value, _value, _value }
    {}

    template< typename T >
    cVector4< T >::cVector4( const T& _x, const T& _y, const T& _z, const T& _w )
    : m_array{ _x, _y, _z, _w }
    {}

    template< typename T >
    cVector4< T >::cVector4( const cVector4& _other )
    : m_array{ _other.x(), _other.y(), _other.z(), _other.w() }
    {}

    template< typename T >
    cVector4< T >::cVector4( const cVector3< T >& _other, const T& _w )
    : m_array{ _other.x(), _other.y(), _other.z(), _w }
    {}

    template< typename T >
    cVector4< T >::cVector4( const cVector2< T >& _other, const T& _z, const T& _w )
    : m_array{ _other.x(), _other.y(), _z, _w }
    {}

    template< typename T >
    cVector4< T >& cVector4< T >::operator=( const T& _value )
    {
        m_array = { _value, _value, _value, _value };
        return *this;
    }

    template< typename T >
    cVector4< T >& cVector4< T >::operator=( const cVector4& _other )
    {
        if( this != &_other )
            m_array = _other.m_array;
        return *this;
    }

    template< typename T >
    cVector4< T >& cVector4< T >::operator+=( const T& _value )
    {
        x() += _value;
        y() += _value;
        z() += _value;
        w() += _value;
        return *this;
    }

    template< typename T >
    cVector4< T >& cVector4< T >::operator+=( const cVector4& _other )
    {
        if( this != &_other )
        {
            x() += _other.x();
            y() += _other.y();
            z() += _other.z();
            w() += _other.w();
        }
        return *this;
    }

    template< typename T >
    cVector4< T >& cVector4< T >::operator-=( const T& _value )
    {
        x() -= _value;
        y() -= _value;
        z() -= _value;
        w() -= _value;
        return *this;
    }

    template< typename T >
    cVector4< T >& cVector4< T >::operator-=( const cVector4& _other )
    {
        if( this != &_other )
        {
            x() -= _other.x();
            y() -= _other.y();
            z() -= _other.z();
            w() -= _other.w();
        }
        return *this;
    }

    template< typename T >
    cVector4< T >& cVector4< T >::operator*=( const T& _value )
    {
        x() *= _value;
        y() *= _value;
        z() *= _value;
        w() *= _value;
        return *this;
    }

    template< typename T >
    cVector4< T >& cVector4< T >::operator*=( const cVector4& _other )
    {
        if( this != &_other )
        {
            x() *= _other.x();
            y() *= _other.y();
            z() *= _other.z();
            w() *= _other.w();
        }
        return *this;
    }

    template< typename T >
    cVector4< T >& cVector4< T >::operator/=( const T& _value )
    {
        x() /= _value;
        y() /= _value;
        z() /= _value;
        w() /= _value;
        return *this;
    }

    template< typename T >
    cVector4< T >& cVector4< T >::operator/=( const cVector4& _other )
    {
        if( this != &_other )
        {
            x() /= _other.x();
            y() /= _other.y();
            z() /= _other.z();
            w() /= _other.w();
        }
        return *this;
    }

    template< typename T >
    cVector4< T >& cVector4< T >::normalize()
    {
        double magnitude = length();
        x() /= magnitude;
        y() /= magnitude;
        z() /= magnitude;
        w() /= magnitude;
        return *this;
    }

    template< typename T >
    cVector4< T > cVector4< T >::normalized()
    {
        double magnitude = length();
        return cVector4( x() / magnitude, y() / magnitude, z() / magnitude, w() / magnitude );
    }

    typedef cVector4< float > cVector4f;
}
