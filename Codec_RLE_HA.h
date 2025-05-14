#pragma once

#include <cstdint>

#include "../helpers/FileUtils.h"
#include "../helpers/StringL.h"
#include "../helpers/Array.h"

#include "CodecRLE.h"
#include "CodecHA.h"

template <typename charType>
class Codec_RLE_HA: CodecRLE<charType>, 
                    CodecHA<charType>
{
private:
    Codec_RLE_HA() = default;
public:
    static void Encode(const StringL<charType>& inputStr, std::ofstream& outputFile, const bool useUTF8);
    static StringL<charType> Decode(std::ifstream& inputFile, const bool useUTF8);
protected:
    struct data {
        typename CodecHA<charType>::data dataHA;
        data() = default;
    };
};

template <typename charType>
void Codec_RLE_HA<charType>::Encode(const StringL<charType>& inputStr, std::ofstream& outputFile, const bool useUTF8)
{
    Codec_RLE_HA<charType>::data data;

    auto rleData = CodecRLE<charType>::encodeToData(inputStr);
    std::cout << "\tRLE done." << std::endl;

    StringL<charType> rleStr = rleData.toString();
    rleData.encodedNumbers.free_memory();
    rleData.encodedChars.free_memory();
    auto haData = CodecHA<charType>::encodeToData(rleStr);
    rleStr.free_memory();
    data.dataHA = haData;
    std::cout << "\tHA done." << std::endl;

    CodecHA<charType>::encodeData(outputFile, data.dataHA, useUTF8);
}

template <typename charType>
StringL<charType> Codec_RLE_HA<charType>::Decode(std::ifstream& inputFile, const bool useUTF8)
{
    StringL<charType> strHA = CodecHA<charType>::Decode(inputFile, useUTF8);
    std::cout << "\tHA done." << std::endl;

    auto dataRLE = CodecRLE<charType>::data::fromString(strHA);
    strHA.free_memory();
    StringL<charType> decodedStr = CodecRLE<charType>::decodeData(dataRLE);
    dataRLE.encodedNumbers.free_memory();
    dataRLE.encodedChars.free_memory();
    std::cout << "\tRLE done." << std::endl;

    return decodedStr;
}