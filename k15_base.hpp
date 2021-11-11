#ifndef K15_BASE_INCLUDE
#define K15_BASE_INCLUDE

#define K15_ASSERT(x) if(!(x)){  }
#define K15_UNUSED_VARIABLE(x) (void)(x)

#include "stdio.h"

namespace k15
{
    typedef unsigned char   bool8;
    typedef unsigned int    bool32;
    typedef unsigned char   byte;
    typedef unsigned int    uint32;
    typedef unsigned short  uint16;
    typedef unsigned char   uint8;
    typedef float           float32;
    typedef double          float64;

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
        out_of_memory,
        internal
    };

    string_view getErrorString( error_id errorId)
    {
        switch( errorId )
        {
            case error_id::success:
                return "success";

            case error_id::out_of_memory:
                return "out of memory";
            
            case error_id::internal:
                return "internal";
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

        T getValue() const
        {
            K15_ASSERT(error == error_id::success);
            return value;
        }

        error_id getError() const
        {
            return errorId;
        }

        bool hasError() const
        {
            return errorId != error_id::success;
        }

        bool isOk() const
        {
            return errorId == error_id::success;
        }

        string_view getErrorString() const
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

        bool hasError() const
        {
            return errorId != error_id::success;
        }

        bool isOk() const
        {
            return errorId == error_id::success;
        }

        string_view getErrorString() const
        {
            return k15::getErrorString( errorId );
        }

    private:
        error_id errorId;
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

    size_t copyMemoryNonOverlapping8( void* pDestination, size_t destinationSizeInBytes, const void* pSource, size_t sourceSizeInBytes )
    {
        const size_t numberOfBytesToCopy = getMin( destinationSizeInBytes, sourceSizeInBytes );
        size_t byteIndex = 0u;

        byte* pDestinationBuffer = (byte*)pDestination;
        byte* pSourceBuffer = (byte*)pSource;

        while( byteIndex < numberOfBytesToCopy )
        {
            *pDestinationBuffer++ = *pSourceBuffer++;
            ++byteIndex;
        }

        return numberOfBytesToCopy;
    }

    template< typename T >
    void fillMemory( void* pDestination, T value, size_t elementCount )
    {
        T* pDestinationBuffer = (T*)pDestination;
        while( elementCount-- > 0u )
        {
            *pDestinationBuffer++ = value;
        }
    }

    template< typename T >
    struct slice
    {
    protected:
        typedef bool (growBufferFunction)( slice< T >* pSlice, uint32 capacity  );
    
    public:
        slice()
        {
            pBuffer     = nullptr;
            capacity    = 0u;
            size        = 0u;
        }
        ~slice()
        {

        }

        T* getStart()
        {
            return pBuffer;
        }

        T* getEnd()
        {
            return pBuffer + capacity;
        }

        const T* getStart() const
        {
            return pBuffer;
        }

        const T* getEnd() const
        {
            return pBuffer + capacity;
        }

        size_t getSize() const
        {
            return size;
        }

        T* pushBack( T value )
        {
            T* pValue = pushBack();
            *pValue = value;

            return pValue;
        }

        T* pushBack()
        {
            return pushBackRange(1u);
        }

        T* pushBackRange(uint32 elementCount)
        {
            if( size + elementCount >= capacity )
            {
                if( !pGrowBufferFunction(this, size + elementCount) )
                {
                    return nullptr;
                }
            }

            K15_ASSERT(pBuffer != nullptr);

            T* pDataStart = pBuffer + size;
            size += elementCount;
            return pDataStart;
        }

    protected:
        uint32 capacity;
        uint32 size;

        T*                  pBuffer;
        growBufferFunction* pGrowBufferFunction;
    };

    template< typename T, uint32 Size = 0 >
    struct dynamic_array : slice< T >
    {
    public:
        dynamic_array()
        {
            pBuffer = staticBuffer;
            capacity = Size;
            size = 0;
            pGrowBufferFunction = dynamic_array<T, Size>::growBuffer;
        }
        ~dynamic_array()
        {
            freeBuffer();
        }

        void freeBuffer()
        {
            if (pBuffer != staticBuffer)
            {
                free(pBuffer);
                pBuffer = nullptr;
            }
        }

        static bool growBuffer( slice< T >* pSlice, uint32 capacity  )
        {
            dynamic_array<T, Size>* pArray = (dynamic_array<T, Size>*)pSlice;
            const int newCapacity = getMax(capacity, pArray->capacity * 2);

            if( newCapacity < Size )
            {
                return true;
            }

            const size_t newBufferSizeInBytes = sizeof(T) * newCapacity;
            T* pNewBuffer = (T*)malloc( newBufferSizeInBytes );
            if (pNewBuffer == nullptr)
            {
                return false;
            }

            copyMemoryNonOverlapping8( pNewBuffer, newBufferSizeInBytes, pArray->pBuffer, pArray->size );
            pArray->freeBuffer();

            pArray->capacity = newCapacity;
            pArray->pBuffer = pNewBuffer;

            return true;
        }

    private:
        T staticBuffer[Size];
    };

    template< typename T >
    struct dynamic_array< T, 0u > : slice< T >
    {
    public:
        dynamic_array()
        {
            pBuffer     = nullptr;
            capacity    = 0u;
            size        = 0;
            pGrowBufferFunction = dynamic_array< T >::growBuffer;
        }

        ~dynamic_array()
        {
            freeBuffer();
        }

        void freeBuffer()
        {
            free(pBuffer);
            pBuffer = nullptr;
        }

        static bool growBuffer( slice< T >* pSlice, uint32 capacity )
        {
            dynamic_array< T >* pArray = (dynamic_array< T >*)pSlice;
            const int newCapacity = capacity;

            T* pNewBuffer = (T*)malloc(sizeof(T) * newCapacity);
            if (pNewBuffer == nullptr)
            {
                return false;
            }

            memcpy( pNewBuffer, pArray->pBuffer, pArray->size );
            pArray->freeBuffer();

            pArray->capacity = newCapacity;
            pArray->pBuffer = pNewBuffer;

            return true;
        }
    };

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

    result< void > formatString( slice< char >* pTarget, const string_view& value )
    {
        char* pBuffer = pTarget->pushBackRange( value.getLength() );
        if( pBuffer == nullptr )
        {
            return error_id::out_of_memory;
        }
        const size_t bytesCopied = copyMemoryNonOverlapping8( (byte*)pBuffer, value.getLength(), (const byte*)value.getStart(), value.getLength() );
        return bytesCopied == value.getLength() ? error_id::success : error_id::internal;
    }

    result< void > formatString( slice< char >* pTarget, const error_id& errorId)
    {
        return formatString( pTarget, getErrorString( errorId ) );
    }

    template< typename... Args >
    result< void > formatString( slice< char >* pTarget, const string_view& format, Args... args )
    {
        const size_t formatLength = format.getLength();
        size_t formatCharIndex = 0u;

        while( formatCharIndex < formatLength )
        {
            const char formatChar = format[ formatCharIndex ];
            if( formatChar == '%' )
            {
                const format_type type = parseFormatType( format, formatCharIndex );
                if (type.typeId == format_type_id::stringType)
                {
                    const result< void > formatResult = formatString( pTarget, args... );
                    if ( formatResult.hasError() )
                    {
                        return formatResult;
                    }
                }

                formatCharIndex += type.formatLength;
            }
            else
            {
                if( pTarget->pushBack( formatChar ) == nullptr )
                {
                    return error_id::out_of_memory;
                }

                ++formatCharIndex;
            }
        }

        pTarget->pushBack( 0 );
        return error_id::success;
    }

    template< typename... Args >
    result< void > printFormattedString( const string_view& format, Args... args )
    {
        dynamic_array<char, 128> textBuffer;
        const result< void > formatStringResult = formatString( &textBuffer, format, args... );
        if (formatStringResult.hasError())
        {
            return formatStringResult;
        }

        fwrite( textBuffer.getStart(), 1u, textBuffer.getSize(), stdout );
        return error_id::success;
    }
}

#endif //K15_BASE_INCLUDE