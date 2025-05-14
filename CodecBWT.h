#pragma once

#include <string>
#include <cstdint>

#include "../helpers/FileUtils.h"
#include "../helpers/CodecUTF8.h"
#include "../helpers/SuffixArray.h"
#include "../helpers/StringL.h"
#include "../helpers/Pair.h"

template <typename charType>
class CodecBWT
{
public:
    static void Encode(const StringL<charType>& inputStr, std::ofstream& outputFile, const bool useUTF8);
    static StringL<charType> Decode(std::ifstream& inputFile, const bool useUTF8);
private:
    CodecBWT() = default;
protected:
    struct data {
        uint32_t index;
        uint32_t encodedStrLength; 
        StringL<charType> encodedStr;
        data() = default;
        data(const uint32_t _index, const uint32_t _encodedStrLength, const StringL<charType>& _encodedStr) : 
            index(_index), encodedStrLength(_encodedStrLength), encodedStr(_encodedStr) {}
    };

    static data encodeToData(const StringL<charType>& inputStr);
    static void encodeData(std::ofstream& outputFile, const data& data, const bool useUTF8);
    
    static StringL<charType> decodeData(const data& data);
};

template <typename charType>
void CodecBWT<charType>::Encode(const StringL<charType>& inputStr, std::ofstream& outputFile, const bool useUTF8)
{
    const charType endChar = '\0';
    Array<int> suffixArray = buildSuffixArray(inputStr, endChar);

    uint32_t index;
    StringL<charType> encodedStr(inputStr.size() + 1);

    for (size_t i = 0; i < suffixArray.size(); ++i) {
        size_t ind = (suffixArray[i] == 0) ? (inputStr.size() + 1 - 1) : (suffixArray[i] - 1);
        encodedStr.push_back( ind == inputStr.size() ? endChar : inputStr[ind] );
        if (suffixArray[i] == 0) {
            index = i;
        }
    }

    FileUtils::AppendValueBinary(outputFile, index);
    FileUtils::AppendValueBinary(outputFile, static_cast<uint32_t>(inputStr.size()) + 1);
    if (useUTF8) {
        for (const auto& c : encodedStr)
            CodecUTF8::EncodeCharToBinaryFile(outputFile, c);
    } else {
        for (const auto& c : encodedStr)
            FileUtils::AppendValueBinary(outputFile, c);
    }
}

template <typename charType>
StringL<charType> CodecBWT<charType>::Decode(std::ifstream& inputFile, const bool useUTF8)
{
    uint32_t index = FileUtils::ReadValueBinary<uint32_t>(inputFile);
    uint32_t encodedStrLength = FileUtils::ReadValueBinary<uint32_t>(inputFile);

    StringL<charType> encodedStr(encodedStrLength);
    if (useUTF8) {
        for (uint32_t i = 0; i < encodedStrLength; ++i) {
            encodedStr.push_back(CodecUTF8::DecodeCharFromBinaryFile<charType>(inputFile));
        }
    } else {
        for (uint32_t i = 0; i < encodedStrLength; ++i) {
            encodedStr.push_back(FileUtils::ReadValueBinary<charType>(inputFile));
        }
    }

    Array<Pair<charType, unsigned int>> P(encodedStrLength);
    for (uint32_t i = 0; i < encodedStrLength; ++i) {
        P.push_back(Pair(encodedStr[i], i));
    }

    auto cmp = [](const Pair<charType, unsigned int>& a, const Pair<charType, unsigned int>& b) {
        return a.first < b.first;
    };
    std::stable_sort(P.begin(), P.end(), cmp); 

    StringL<charType> decodedStr(encodedStrLength - 1);
    for (uint32_t i = 0; i < encodedStrLength - 1; ++i) {
        index = P[index].second;
        decodedStr.push_back(encodedStr[index]);
    }

    return decodedStr;
}

template <typename charType>
typename CodecBWT<charType>::data CodecBWT<charType>::encodeToData(const StringL<charType>& inputStr)
{
    const charType endChar = '\0';

    Array<int> suffixArray = buildSuffixArray(inputStr, endChar);

    uint32_t index;
    StringL<charType> encodedStr(inputStr.size() + 1); 

    for (size_t i = 0; i < suffixArray.size(); ++i) {
        size_t ind = (suffixArray[i] == 0) ? (inputStr.size() + 1 - 1) : (suffixArray[i] - 1);
        encodedStr.push_back( ind == inputStr.size() ? endChar : inputStr[ind] );
        if (suffixArray[i] == 0) {
            index = i;
        }
    }

    return data(index, inputStr.size() + 1, encodedStr);
}

template <typename charType>
void CodecBWT<charType>::encodeData(std::ofstream& outputFile, const data& data, const bool useUTF8)
{
    FileUtils::AppendValueBinary(outputFile, data.index);
    FileUtils::AppendValueBinary(outputFile, data.encodedStrLength);
    if (useUTF8) {
        for (const auto& c : data.encodedStr) {
            CodecUTF8::EncodeCharToBinaryFile(outputFile, c);
        }
    } else {
        for (const auto& c : data.encodedStr) {
            FileUtils::AppendValueBinary(outputFile, c);
        }
    }
}

template <typename charType>
StringL<charType> CodecBWT<charType>::decodeData(const data& data)
{
    Array<Pair<charType, uint32_t>> P(data.encodedStrLength);
    for (uint32_t i = 0; i < data.encodedStrLength; ++i) {
        P.push_back(Pair(data.encodedStr[i], i));
    }

    auto cmp = [](const Pair<charType, unsigned int>& a, const Pair<charType, unsigned int>& b) {
        return a.first < b.first;
    };
    std::stable_sort(P.begin(), P.end(), cmp);

    StringL<charType> decodedStr;
    uint32_t index = data.index;
    for (size_t i = 0; i < data.encodedStr.size() - 1; ++i) { 
        index = P[index].second;                             
        decodedStr.push_back(data.encodedStr[index]);
    }

    return decodedStr;
}