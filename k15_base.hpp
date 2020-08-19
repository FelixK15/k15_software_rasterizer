#ifndef K15_BASE_INCLUDE
#define K15_BASE_INCLUDE

#define K15_ASSERT(x) if(!(x)){  }
#define K15_UNUSED_VARIABLE(x) (void)(x)

namespace k15
{
    typedef unsigned char   bool8;
    typedef unsigned int    bool32;
    typedef unsigned char   byte;
    typedef unsigned int    uint32;
    typedef unsigned short  uint16;
    typedef unsigned char   uint8;

    size_t getStringLength( const char* pString )
    {
        size_t length = 0u;
        while( *pString++ != 0 )
        {
            ++length;
        }

        return length;
    }

    struct string_view
    {
    public:
        string_view( const char* pString )
        {
            pData = pString;
            length = getStringLength( pString );
        }
        string_view( const char* pString, size_t stringLength )
        {
            pData = pString;
            length = stringLength;
        }

        size_t getLength() const
        {
            return length;
        }

        const char* getStart() const
        {
            return pData;
        }

        const char* getEnd() const
        {
            return pData + length;
        }

        bool8 isEmpty() const
        {
            return length == 0u;
        }

        char operator[](size_t index) const
        {
            K15_ASSERT(index < length);
            return pData[ index ];
        }

    public:
        static const string_view empty;

    private:
        const char* pData;
        size_t      length;
    };

    const string_view string_view::empty = string_view("");

    enum class error_id
    {
        success,
        out_of_memory
    };

    string_view getErrorString( error_id errorId)
    {
        switch( errorId )
        {
            case error_id::success:
                return "success";

            case error_id::out_of_memory:
                return "out of memory";
        }

        return string_view::empty;
    }

    template< typename T >
    struct result
    {
    public:
        result( T value )
        {
            this.value = value;
            errorId = error_id::success;
        }

        result( error_id error )
        {
            errorId = error;
        }

        T        getValue() const
        {
            K15_ASSERT(error == error_id::success);
            return value;
        }

        error_id getError() const
        {
            return errorId;
        }

        bool     hasError() const
        {
            return errorId != error_id::success;
        }

        bool     isOk() const
        {
            return errorId == error_id::success;
        }

        string_view     getErrorString() const
        {
            return k15::getErrorString( errorId );
        }

    private:
        T           value;
        error_id    errorId;
    };

    template<>
    struct result< void >
    {
    public:
        result( error_id error )
        {
            errorId = error;
        }

        error_id getError() const
        {
            return errorId;
        }

        bool     hasError() const
        {
            return errorId != error_id::success;
        }

        bool     isOk() const
        {
            return errorId == error_id::success;
        }

        string_view     getErrorString() const
        {
            return k15::getErrorString( errorId );
        }

    private:
        error_id    errorId;
    };

    template< typename T >
    T getMin(T a, T b)
    {
        return a > b ? b : a;
    }

    template< typename T >
    T getMax(T a, T b)
    {
        return a > b ? a : b;
    }

    size_t copyMemoryNonOverlapping8( byte* pDestination, size_t destinationSizeInBytes, const byte* pSource, size_t sourceSizeInBytes )
    {
        const size_t numberOfBytesToCopy = getMin( destinationSizeInBytes, sourceSizeInBytes );
        size_t byteIndex = 0u;
        while( byteIndex < numberOfBytesToCopy )
        {
            *pDestination++ = *pSource++;
            ++byteIndex;
        }

        return numberOfBytesToCopy;
    }

    enum class format_type_id
    {
        decimalType,
        floatType,
        stringType,

        invalidType
    };

    struct format_type
    {
        format_type_id              typeId;
        size_t                      formatLength;

        static format_type create( format_type_id id, size_t formatLength )
        {
            format_type formatType;
            formatType.typeId = id;
            formatType.formatLength = formatLength;

            return formatType;
        }

        const static format_type    invalid;
    };

    const format_type format_type::invalid = format_type::create( format_type_id::invalidType, 0u );

    format_type parseFormatType( const string_view& format, size_t offsetInChars )
    {
        if( format[ offsetInChars ] != '%')
        {
            return format_type::invalid;
        }

        if( format[ offsetInChars + 1] == 's' )
        {
            return format_type::create( format_type_id::stringType, 2u );
        }

        return format_type::invalid;
    }

    size_t formatString( char* pBuffer, size_t bufferSizeInBytes, const string_view& value )
    {
        return copyMemoryNonOverlapping8( (byte*)pBuffer, bufferSizeInBytes, (const byte*)value.getStart(), value.getLength() );
    }

    template< typename... Args >
    error_id formatString( char* pBuffer, size_t bufferSizeInBytes, const string_view& format, Args... args )
    {
        const char* pBufferEnd = pBuffer + bufferSizeInBytes;
        const size_t formatLength = format.getLength();
        size_t formatCharIndex = 0u;

        while( formatCharIndex < formatLength && pBuffer < pBufferEnd )
        {
            const char formatChar = format[ formatCharIndex ];
            if( formatChar == '%' )
            {
                const format_type type = parseFormatType( format, formatCharIndex );
                if (type.typeId == format_type_id::stringType)
                {
                    const size_t charsWritten = formatString( pBuffer, bufferSizeInBytes, args... );
                    pBuffer = pBuffer + charsWritten;

                }

                formatCharIndex += type.formatLength;
            }
            else
            {
                *pBuffer++ = formatChar;
                ++formatCharIndex;
            }
        }

        if( pBuffer < pBufferEnd )
        {
            *pBuffer = 0;
        }
        return error_id::success;
    }
}

#endif //K15_BASE_INCLUDE