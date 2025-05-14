#pragma once

#include <cstdint>

#include "../helpers/FileUtils.h"
#include "../helpers/CodecUTF8.h"
#include "../helpers/StringL.h"
#include "../helpers/Array.h"

#include "../compressor/CompressorSettings.h"

template <typename charType>
class CodecLZ77
{
public:
    static void Encode(const StringL<charType>& text, std::ofstream& outputFile, const bool useUTF8);
    static StringL<charType> Decode(std::ifstream& inputFile, const bool useUTF8);
private:
    CodecLZ77() = default;
    static int find(const StringL<charType>& target, const StringL<charType>& original, const uint32_t startIndex, const uint32_t endIndex);

    const static uint32_t lookaheadBufferSize = 128;
protected:
    struct data {
        uint32_t inputStrLength;
        Array<uint16_t> offsets;
        Array<uint8_t> lengths;
        StringL<charType> chars;

        data() = default;
        data(const uint32_t inputStrLength_, const Array<uint16_t>& offsets_, const Array<uint8_t>& lengths_, const StringL<charType> chars_) :
            inputStrLength(inputStrLength_), lengths(lengths_), offsets(offsets_), chars(chars_) {}

        const StringL<charType> toString() const;
        static const data fromString(const StringL<charType>& str, const uint32_t inputStrLength);
    };

    static data encodeToData(const StringL<charType>& inputStr);
    static void encodeData(std::ofstream& outputFile, const data& data, const bool useUTF8);
    static StringL<charType> decodeData(const data& data);
};

template <typename charType>
void CodecLZ77<charType>::Encode(const StringL<charType>& text, std::ofstream& outputFile, const bool useUTF8)
{
    FileUtils::AppendValueBinary(outputFile, static_cast<uint32_t>(text.size()));

    const uint32_t searchBufferSize = CompressorSettings::GetLZ77SearchBufferSize();
    const uint32_t lookaheadBufferSize = CodecLZ77<charType>::lookaheadBufferSize;

    uint32_t i = 0; 

    while (i < text.size())
    {
        StringL<charType> maxStr(lookaheadBufferSize); 
        bool flag = true; 

        uint32_t searchBufferStart = (i > searchBufferSize) ? (i - searchBufferSize) : (0u);
        uint32_t searchBufferEnd = i;
        uint32_t offset = 0;
        uint8_t length = 0;
        int32_t index = searchBufferStart;

        while (flag && (i < text.size()) && (maxStr.size() < lookaheadBufferSize))
        {
            maxStr.push_back(text[i]);

            index = find(maxStr, text, index, searchBufferEnd);
            if (index == -1)
            {
                break;
            }

            ++length;
            offset = searchBufferEnd - index;
            ++i;
        }
        FileUtils::AppendValueBinary(outputFile, static_cast<uint16_t>(offset));
        FileUtils::AppendValueBinary(outputFile, static_cast<uint8_t>(length));
        if (length == 0) {
            if (useUTF8) {
                CodecUTF8::EncodeCharToBinaryFile(outputFile, text[i++]);
            } else {
                FileUtils::AppendValueBinary(outputFile, text[i++]);
            }
        }
    }
}

template <typename charType>
StringL<charType> CodecLZ77<charType>::Decode(std::ifstream& inputFile, const bool useUTF8)
{
    uint32_t inputStrLength = FileUtils::ReadValueBinary<uint32_t>(inputFile);

    StringL<charType> decoded(inputStrLength);
    uint32_t stringPointer = 0; 

    uint32_t offset, length;
    while (decoded.size() < inputStrLength)
    {
        offset = FileUtils::ReadValueBinary<uint16_t>(inputFile);
        length = FileUtils::ReadValueBinary<uint8_t>(inputFile);
        if (length == 0)
        {
            if (useUTF8) {
                decoded.push_back(CodecUTF8::DecodeCharFromBinaryFile<charType>(inputFile));
            } else {
                decoded.push_back(FileUtils::ReadValueBinary<charType>(inputFile));
            }
            ++stringPointer;
        }
        else
        {
            uint32_t start = stringPointer - offset;
            uint32_t end = stringPointer - offset + length;
            stringPointer += length;
            for (uint32_t j = start; j < end; ++j) {
                decoded.push_back(decoded[j]);
            }
        }
    }

    return decoded;
}

template <typename charType>
int CodecLZ77<charType>::find(const StringL<charType>& target, const StringL<charType>& original, const uint32_t startIndex, const uint32_t endIndex)
{
    uint32_t i = startIndex;
    uint32_t j = 0; 
    while (i < endIndex)
    {
        if (original[i] == target[j])
        {
            ++j;
            if (j == target.size()) return i - j + 1;
        }
        else
        {
            i -= j;
            j = 0u;
        }

        ++i;
    }

    return -1;
}

template <typename charType>
typename CodecLZ77<charType>::data CodecLZ77<charType>::encodeToData(const StringL<charType>& text)
{
    const uint32_t searchBufferSize = CompressorSettings::GetLZ77SearchBufferSize();
    const uint32_t lookaheadBufferSize = CodecLZ77<charType>::lookaheadBufferSize;

    Array<uint8_t> lengths(text.size());
    Array<uint16_t> offsets(text.size());
    StringL<charType> chars(text.size());

    uint32_t i = 0; 
    while (i < text.size())
    {
        StringL<charType> maxStr(lookaheadBufferSize); 
        bool flag = true; 

        uint32_t searchBufferStart = (i > searchBufferSize) ? (i - searchBufferSize) : (0u);
        uint32_t searchBufferEnd = i;
        uint32_t offset = 0;
        uint8_t length = 0;
        int32_t index = searchBufferStart; 

        while (flag && (i < text.size()) && (maxStr.size() < lookaheadBufferSize))
        {
            maxStr.push_back(text[i]);

            index = find(maxStr, text, index, searchBufferEnd);
            if (index == -1)
            {
                break;
            }

            ++length;
            offset = searchBufferEnd - index;
            ++i;
        }

        offsets.push_back(static_cast<uint16_t>(offset));
        lengths.push_back(static_cast<uint8_t>(length));
        if (length == 0) { chars.push_back(text[i++]); }
    }
    
    return data(text.size(), offsets, lengths, chars);
}

template <typename charType>
void CodecLZ77<charType>::encodeData(std::ofstream& outputFile, const data& data, const bool useUTF8)
{
    FileUtils::AppendValueBinary(outputFile, data.inputStrLength);

    size_t charsPointer = 0;

    for (uint32_t i = 0; i < data.lengths.size(); ++i)
    {
        FileUtils::AppendValueBinary(outputFile, data.offsets[i]);
        FileUtils::AppendValueBinary(outputFile, data.lengths[i]);
        if (data.lengths[i] == 0) {
            if (useUTF8) {
                CodecUTF8::EncodeCharToBinaryFile(outputFile, data.chars[charsPointer++]);
            } else {
                FileUtils::AppendValueBinary(outputFile, data.chars[charsPointer++]);
            }
        }
    }
}

template <typename charType>
StringL<charType> CodecLZ77<charType>::decodeData(const data& data)
{
    StringL<charType> decoded(data.inputStrLength);
    size_t charsPointer = 0;
    uint32_t stringPointer = 0; 

    uint32_t i = 0;
    while (decoded.size() < data.inputStrLength)
    {
        if (data.lengths[i] == 0)
        {
            decoded.push_back(data.chars[charsPointer++]);
            ++stringPointer;
        }
        else
        {
            uint32_t start = stringPointer - data.offsets[i];
            uint32_t end = stringPointer - data.offsets[i] + data.lengths[i];
            stringPointer += data.lengths[i];
            for (uint32_t j = start; j < end; ++j) {
                decoded.push_back(decoded[j]);
            }
        }
        ++i;
    }

    return decoded;
}


template <typename charType>
const StringL<charType> CodecLZ77<charType>::data::toString() const
{
    StringL<charType> result(offsets.size() + lengths.size() + chars.size());
    size_t charsPointer = 0;

    if (std::is_same<charType, unsigned char>::value)
    {
        for (size_t i = 0; i < lengths.size(); ++i)
        {
            result.push_back(static_cast<unsigned char>(offsets[i] >> 8));
            result.push_back(static_cast<unsigned char>(offsets[i] & 0b11111111));
            result.push_back(static_cast<unsigned char>(lengths[i]));
            if (lengths[i] == 0) { result.push_back(chars[charsPointer++]); }
        }
    }
    else if ((std::is_same<charType, char16_t>::value) || (std::is_same<charType, unsigned short>::value))
    {
        for (size_t i = 0; i < lengths.size(); ++i)
        {
            result.push_back(static_cast<unsigned short>(offsets[i]));
            result.push_back(static_cast<unsigned short>(lengths[i]));
            if (lengths[i] == 0) { result.push_back(chars[charsPointer++]); }
        }
    }
    else if ((std::is_same<charType, char32_t>::value) || (std::is_same<charType, unsigned int>::value))
    {
        for (size_t i = 0; i < lengths.size(); ++i)
        {
            result.push_back(static_cast<unsigned int>((offsets[i] << 8) + lengths[i]));
            if (lengths[i] == 0) { result.push_back(chars[charsPointer++]); }
        }
    }
    else
    {
        throw std::runtime_error("Error: Unsupported char type in CodecLZ77::data::toString()");
    }

    return result;
}

template <typename charType>
const typename CodecLZ77<charType>::data CodecLZ77<charType>::data::fromString(const StringL<charType>& str, const uint32_t inputStrLength)
{
    data result;
    result.inputStrLength = inputStrLength;
    result.offsets.resize(inputStrLength);
    result.lengths.resize(inputStrLength);
    result.chars.resize(inputStrLength);

    uint16_t offset;
    uint8_t length;
    size_t i = 0; 
    if (std::is_same<charType, unsigned char>::value)
    {
        while (i < str.size())
        {
            offset = (str[i] << 8) + str[i + 1];
            length = str[i + 2];
            result.offsets.push_back(offset);
            result.lengths.push_back(length);
            i += 3;
            if (length == 0) { result.chars.push_back(str[i++]); }
        }
    }
    else if ((std::is_same<charType, char16_t>::value) || (std::is_same<charType, unsigned short>::value))
    {
        while (i < str.size())
        {
            offset = static_cast<uint16_t>(str[i++]);
            length = static_cast<uint8_t>(str[i++]);
            result.lengths.push_back(length);
            result.offsets.push_back(offset);
            if (length == 0) { result.chars.push_back(str[i++]); }
        }
    }
    else if ((std::is_same<charType, char32_t>::value) || (std::is_same<charType, unsigned int>::value))
    {
        while (i < str.size())
        {
            offset = static_cast<uint16_t>(str[i] & 0b00000000111111111111111100000000);
            length = static_cast<uint8_t>(str[i++] & 0b00000000000000000000000011111111);
            result.offsets.push_back(offset);
            result.lengths.push_back(length);
            if (length == 0) { result.chars.push_back(str[i++]); }
        }
    }
    else
    {
        throw std::runtime_error("Error: Unsupported char type in CodecLZ77::data::fromString()");
    }

    return result;
}