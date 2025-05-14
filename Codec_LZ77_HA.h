#pragma once

#include <cstdint>

#include "../helpers/FileUtils.h"
#include "../helpers/StringL.h"
#include "../helpers/Array.h"

#include "CodecLZ77.h"
#include "CodecHA.h"

template <typename charType>
class Codec_LZ77_HA: CodecLZ77<charType>, 
                     CodecHA<charType>
{
private:
    Codec_LZ77_HA() = default;
public:
    static void Encode(const StringL<charType>& inputStr, std::ofstream& outputFile, const bool useUTF8);
    static StringL<charType> Decode(std::ifstream& inputFile, const bool useUTF8);
protected:
    struct data {
        uint32_t inputStrLength;
        typename CodecHA<charType>::data dataHA;
        data() = default;
    };
};

template <typename charType>
void Codec_LZ77_HA<charType>::Encode(const StringL<charType>& inputStr, std::ofstream& outputFile, const bool useUTF8)
{
    Codec_LZ77_HA<charType>::data data;

    data.inputStrLength = inputStr.size();

    auto lz77Data = CodecLZ77<charType>::encodeToData(inputStr);
    StringL<charType> strLZ77 = lz77Data.toString();
    lz77Data.lengths.free_memory();
    lz77Data.offsets.free_memory();
    lz77Data.chars.free_memory();
    std::cout << "\tLZ77 done." << std::endl;

    auto haData = CodecHA<charType>::encodeToData(strLZ77);
    strLZ77.free_memory();
    data.dataHA = haData;
    std::cout << "\tHA done." << std::endl;

    CodecHA<charType>::encodeData(outputFile, data.dataHA, useUTF8);
    FileUtils::AppendValueBinary(outputFile, data.inputStrLength);
}

template <typename charType>
StringL<charType> Codec_LZ77_HA<charType>::Decode(std::ifstream& inputFile, const bool useUTF8)
{
    StringL<charType> strHA = CodecHA<charType>::Decode(inputFile, useUTF8);
    std::cout << "\tHA done." << std::endl;

    uint32_t inputStrLength = FileUtils::ReadValueBinary<uint32_t>(inputFile);
    auto dataLZ77 = CodecLZ77<charType>::data::fromString(strHA, inputStrLength);
    strHA.free_memory();
    StringL<charType> decodedStr = CodecLZ77<charType>::decodeData(dataLZ77);
    std::cout << "\tLZ77 done." << std::endl;

    return decodedStr;
}