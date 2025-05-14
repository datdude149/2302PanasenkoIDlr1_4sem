#pragma once

#include <cstdint>
#include <algorithm>

#include "../helpers/FileUtils.h"
#include "../helpers/CodecUTF8.h"
#include "../helpers/TextUtils.h"
#include "../helpers/StringL.h"
#include "../helpers/Array.h"

template <typename charType>
class CodecMTF
{
public:
    static void Encode(const StringL<charType>& inputStr, std::ofstream& outputFile, const bool useUTF8);
    static StringL<charType> Decode(std::ifstream& inputFile, const bool useUTF8);
private:
    CodecMTF() = default;
    static inline void AlphabetShift(Array<charType>& alphabet, const uint32_t index);
    static inline const uint32_t GetIndex(const Array<charType>& alphabet, const charType c);
protected:
    struct data {
        uint32_t alphabetLength;
        Array<charType> alphabet;
        uint32_t inputStrLength;
        Array<uint32_t> codes;

        data() = default;
        data(const uint32_t _alphabetLength, const Array<charType>& _alphabet, const uint32_t _inputStrLength, const Array<uint32_t>& _codes) : 
            alphabetLength(_alphabetLength), alphabet(_alphabet), inputStrLength(_inputStrLength), codes(_codes) {}

        StringL<charType> toString();
        static Array<uint32_t> codesFromString(const StringL<charType>& str);
    };

    static data encodeToData(const StringL<charType>& inputStr);
    static void encodeData(std::ofstream& outputFile, const data& data, const bool useUTF8);

    static StringL<charType> decodeData(const data& data);
};

template <typename charType>
void CodecMTF<charType>::Encode(const StringL<charType>& inputStr, std::ofstream& outputFile, const bool useUTF8)
{
    Array<charType> alphabet = TextUtils::GetAlphabet<charType>(inputStr);

    Array<uint32_t> codes(inputStr.size());
    uint32_t index;
    for (const auto& c : inputStr) {
        index = GetIndex(alphabet, c);
        codes.push_back(index);
        AlphabetShift(alphabet, index);
    }

    std::sort(alphabet.begin(), alphabet.end());
    FileUtils::AppendValueBinary(outputFile, static_cast<uint32_t>(alphabet.size()));
    if (useUTF8) {
        for (const auto& c : alphabet)
            CodecUTF8::EncodeCharToBinaryFile(outputFile, c);
    } else {
        for (const auto& c : alphabet)
            FileUtils::AppendValueBinary(outputFile, c);
    }
    FileUtils::AppendValueBinary(outputFile, static_cast<uint32_t>(inputStr.size()));
    if (alphabet.size() <= 256) {
        for (const auto& code : codes)
            FileUtils::AppendValueBinary(outputFile, static_cast<uint8_t>(code));
    } else if (alphabet.size() <= 65536) {
        for (const auto& code : codes)
            FileUtils::AppendValueBinary(outputFile, static_cast<uint16_t>(code));
    } else {
        for (const auto& code : codes)
            FileUtils::AppendValueBinary(outputFile, static_cast<uint32_t>(code));
    }
}

template <typename charType>
StringL<charType> CodecMTF<charType>::Decode(std::ifstream& inputFile, const bool useUTF8)
{
    uint32_t alphabetLength = FileUtils::ReadValueBinary<uint32_t>(inputFile);

    Array<charType> alphabet(alphabetLength);
    if (useUTF8) {
        for (uint32_t i = 0; i < alphabetLength; ++i) {
            alphabet.push_back(CodecUTF8::DecodeCharFromBinaryFile<charType>(inputFile));
        }
    } else {
        for (uint32_t i = 0; i < alphabetLength; ++i) {
            alphabet.push_back(FileUtils::ReadValueBinary<charType>(inputFile));
        }
    }

    uint32_t inputStrLength = FileUtils::ReadValueBinary<uint32_t>(inputFile);
    StringL<charType> decodedStr(inputStrLength);

    if (alphabetLength <= 256) {
        uint8_t index;
        for (uint32_t i = 0; i < inputStrLength; ++i) {
            index = FileUtils::ReadValueBinary<uint8_t>(inputFile);
            decodedStr.push_back(alphabet[index]);

            AlphabetShift(alphabet, index);
        }
    } else if (alphabetLength <= 65536) {
        uint16_t index;
        for (uint32_t i = 0; i < inputStrLength; ++i) {
            index = FileUtils::ReadValueBinary<uint16_t>(inputFile);
            decodedStr.push_back(alphabet[index]);

            AlphabetShift(alphabet, index);
        }
    } else {
        uint32_t index;
        for (uint32_t i = 0; i < inputStrLength; ++i) {
            index = FileUtils::ReadValueBinary<uint32_t>(inputFile);
            decodedStr.push_back(alphabet[index]);

            AlphabetShift(alphabet, index);
        }
    }

    return decodedStr;
}
template <typename charType>
inline void CodecMTF<charType>::AlphabetShift(Array<charType>& alphabet, const uint32_t index)
{
    charType temp = alphabet[0], temp2;
    for (uint32_t i = 1; i <= index; ++i) {
        temp2 = alphabet[i];
        alphabet.assign(i, temp);
        temp = temp2;
    }
    alphabet.assign(0, temp);
}

template <typename charType>
const uint32_t CodecMTF<charType>::GetIndex(const Array<charType>& alphabet, const charType c)
{
    for (size_t i = 0; i < alphabet.size(); ++i) {
        if (alphabet[i] == c) {
            return static_cast<uint32_t>(i);
        }
    }
    throw std::runtime_error("CodecMTF::GetIndex(): Something went wrong!");
    return 0;
}
template <typename charType>
typename CodecMTF<charType>::data CodecMTF<charType>::encodeToData(const StringL<charType>& inputStr)
{
    Array<charType> alphabet = TextUtils::GetAlphabet<charType>(inputStr);

    Array<uint32_t> codes(inputStr.size());
    uint32_t index;
    for (const auto& c : inputStr) {
        index = GetIndex(alphabet, c);
        codes.push_back(index);
        AlphabetShift(alphabet, index);
    }

    std::sort(alphabet.begin(), alphabet.end());
    return data(alphabet.size(), alphabet, inputStr.size(), codes);
}

template <typename charType>
void CodecMTF<charType>::encodeData(std::ofstream& outputFile, const data& data, const bool useUTF8)
{
    FileUtils::AppendValueBinary(outputFile, data.alphabetLength);

    if (useUTF8) {
        for (const auto& c : data.alphabet) {
            CodecUTF8::EncodeCharToBinaryFile<charType>(outputFile, c);
        }
    } else {
        for (const auto& c : data.alphabet) {
            FileUtils::AppendValueBinary<charType>(outputFile, c);
        }
    }

    FileUtils::AppendValueBinary(outputFile, data.inputStrLength);
    
    if (data.alphabetLength <= 256) {
        for (const auto& code : data.codes) {
            FileUtils::AppendValueBinary(outputFile, static_cast<uint8_t>(code));
        }
    } else if (data.alphabetLength <= 65536) {
        for (const auto& code : data.codes) {
            FileUtils::AppendValueBinary(outputFile, static_cast<uint16_t>(code));
        }
    } else {
        for (const auto& code : data.codes) {
            FileUtils::AppendValueBinary(outputFile, static_cast<uint32_t>(code));
        }
    }
}

template <typename charType>
StringL<charType> CodecMTF<charType>::decodeData(const data& data)
{
    StringL<charType> decodedStr(data.inputStrLength);
    Array<charType> alphabet = data.alphabet;

    for (const auto& code : data.codes) {
        decodedStr.push_back(alphabet[code]);
        AlphabetShift(alphabet, code);
    }
    return decodedStr;
}

template <typename charType>
StringL<charType> CodecMTF<charType>::data::toString()
{
    StringL<charType> result(inputStrLength);
    for (const uint32_t& code : codes) {
        result.push_back(static_cast<charType>(code));
    }
    return result;
}

template <typename charType>
Array<uint32_t> CodecMTF<charType>::data::codesFromString(const StringL<charType>& str)
{
    Array<uint32_t> codes(str.size());
    for (const auto& c : str) {
        codes.push_back(static_cast<uint32_t>(c));
    }
    return codes;
}