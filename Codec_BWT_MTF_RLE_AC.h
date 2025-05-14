#pragma once

#include <cstdint>

#include "../helpers/FileUtils.h"
#include "../helpers/CodecUTF8.h"
#include "../helpers/StringL.h"
#include "../helpers/Array.h"

#include "CodecBWT.h"
#include "CodecMTF.h"
#include "CodecRLE.h"
#include "CodecAC.h"

template <typename charType>
class Codec_BWT_MTF_RLE_AC: CodecBWT<charType>, 
                            CodecMTF<charType>, 
                            CodecRLE<charType>, 
                            CodecAC<charType>
{
private:
    Codec_BWT_MTF_RLE_AC() = default;
public:
    static void Encode(const StringL<charType>& inputStr, std::ofstream& outputFile, const bool useUTF8);
    static StringL<charType> Decode(std::ifstream& inputFile, const bool useUTF8);
protected:
    struct data {
        uint32_t indexBWT;
        uint32_t alphabetLengthMTF;
        Array<charType> alphabetMTF;
        typename CodecAC<charType>::data dataAC;
        data() = default;
    };

    static data encodeToData(const StringL<charType>& inputStr);
    static void encodeData(std::ofstream& outputFile, const data& data, const bool useUTF8);
};

template <typename charType>
typename Codec_BWT_MTF_RLE_AC<charType>::data Codec_BWT_MTF_RLE_AC<charType>::encodeToData(const StringL<charType>& inputStr)
{
    Codec_BWT_MTF_RLE_AC<charType>::data data;

    auto bwtData = CodecBWT<charType>::encodeToData(inputStr);
    data.indexBWT = bwtData.index;

    auto mtfData = CodecMTF<charType>::encodeToData(bwtData.encodedStr);
    data.alphabetLengthMTF = mtfData.alphabetLength;
    data.alphabetMTF = mtfData.alphabet;
    bwtData.encodedStr.free_memory();

    auto rleData = CodecRLE<charType>::encodeToData(mtfData.toString());
    mtfData.codes.free_memory();
    mtfData.alphabet.free_memory();

    auto acData = CodecAC<charType>::encodeToData(rleData.toString());
    data.dataAC = acData;

    return data;
}

template <typename charType>
void Codec_BWT_MTF_RLE_AC<charType>::encodeData(std::ofstream& outputFile, const data& data, const bool useUTF8)
{
    CodecAC<charType>::encodeData(outputFile, data.dataAC, useUTF8);
    FileUtils::AppendValueBinary(outputFile, data.alphabetLengthMTF);
    if (useUTF8) {
        for (const charType c : data.alphabetMTF)
            CodecUTF8::EncodeCharToBinaryFile(outputFile, c);
    } else {
        for (const charType c : data.alphabetMTF)
            FileUtils::AppendValueBinary(outputFile, c);
    }
    FileUtils::AppendValueBinary(outputFile, data.indexBWT);
}

template <typename charType>
void Codec_BWT_MTF_RLE_AC<charType>::Encode(const StringL<charType>& inputStr, std::ofstream& outputFile, const bool useUTF8)
{
    Codec_BWT_MTF_RLE_AC<charType>::data data;

    auto bwtData = CodecBWT<charType>::encodeToData(inputStr);
    data.indexBWT = bwtData.index;
    std::cout << "\tBWT done." << std::endl;

    auto mtfData = CodecMTF<charType>::encodeToData(bwtData.encodedStr);
    bwtData.encodedStr.free_memory();
    data.alphabetLengthMTF = mtfData.alphabetLength;
    data.alphabetMTF = mtfData.alphabet;
    std::cout << "\tMTF done." << std::endl;
    
    StringL<charType> mtfStr = mtfData.toString();
    mtfData.codes.free_memory();
    mtfData.alphabet.free_memory();
    auto rleData = CodecRLE<charType>::encodeToData(mtfStr);
    mtfStr.free_memory();
    std::cout << "\tRLE done." << std::endl;

    StringL<charType> rleStr = rleData.toString();
    rleData.encodedNumbers.free_memory();
    rleData.encodedChars.free_memory();
    auto acData = CodecAC<charType>::encodeToData(rleStr);
    rleStr.free_memory();
    data.dataAC = acData;
    std::cout << "\tAC done." << std::endl;

    CodecAC<charType>::encodeData(outputFile, data.dataAC, useUTF8);
    FileUtils::AppendValueBinary(outputFile, data.alphabetLengthMTF);
    if (useUTF8) {
        for (const charType c : data.alphabetMTF)
            CodecUTF8::EncodeCharToBinaryFile(outputFile, c);
    } else {
        for (const charType c : data.alphabetMTF)
            FileUtils::AppendValueBinary(outputFile, c);
    }
    FileUtils::AppendValueBinary(outputFile, data.indexBWT);
}

template <typename charType>
StringL<charType> Codec_BWT_MTF_RLE_AC<charType>::Decode(std::ifstream& inputFile, const bool useUTF8)
{
    StringL<charType> strAC = CodecAC<charType>::Decode(inputFile, useUTF8);
    std::cout << "\tAC done." << std::endl;

    auto dataRLE = CodecRLE<charType>::data::fromString(strAC);
    strAC.free_memory();
    StringL<charType> strRLE = CodecRLE<charType>::decodeData(dataRLE);
    dataRLE.encodedNumbers.free_memory();
    dataRLE.encodedChars.free_memory();
    std::cout << "\tRLE done." << std::endl;

    uint32_t alphabetLengthMTF = FileUtils::ReadValueBinary<uint32_t>(inputFile);
    Array<charType> alphabetMTF(alphabetLengthMTF);
    if (useUTF8) {
        while (alphabetMTF.size() < alphabetLengthMTF) {
            alphabetMTF.push_back(CodecUTF8::DecodeCharFromBinaryFile<charType>(inputFile));
        }
    } else {
        while (alphabetMTF.size() < alphabetLengthMTF) {
            alphabetMTF.push_back(FileUtils::ReadValueBinary<charType>(inputFile));
        }
    }
    uint32_t strLengthtMTF = strRLE.size();
    Array<uint32_t> codesMTF = CodecMTF<charType>::data::codesFromString(strRLE);
    strRLE.free_memory();
    StringL<charType> strMTF = CodecMTF<charType>::decodeData(typename CodecMTF<charType>::data(alphabetLengthMTF, alphabetMTF, strLengthtMTF, codesMTF));
    alphabetMTF.free_memory();
    codesMTF.free_memory();
    std::cout << "\tMTF done." << std::endl;

    uint32_t index = FileUtils::ReadValueBinary<uint32_t>(inputFile);
    StringL<charType> decodedStr = CodecBWT<charType>::decodeData(typename CodecBWT<charType>::data(index, strMTF.size(), strMTF));
    std::cout << "\tBWT done." << std::endl;

    return decodedStr;
}