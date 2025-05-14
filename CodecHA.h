#pragma once

#include <cstdint>
#include <map>

#include "../helpers/FileUtils.h"
#include "../helpers/CodecUTF8.h"
#include "../helpers/HuffmanTree.h"
#include "../helpers/TextUtils.h"
#include "../helpers/BinaryUtils.h"
#include "../helpers/BitArray.h"
#include "../helpers/StringL.h"
#include "../helpers/Array.h"

#include "../compressor/CompressorSettings.h"

template <typename charType>
class CodecHA
{
public:
    static void Encode(StringL<charType>& inputStr, std::ofstream& outputFile, const bool useUTF8);
    static StringL<charType> Decode(std::ifstream& inputFile, const bool useUTF8);
private:
    CodecHA() = default;

    static void sortInParallel(Array<charType>& alphabet, Array<uint32_t>& frequencies);
    static void encodeNumbersEffectively(std::ofstream& outputFile, const Array<uint32_t>& numbers);
    static Array<uint32_t> decodeNumbersEffectively(std::ifstream& inputFile, const uint16_t numberOfElements);
protected:
    struct data_local {
        uint16_t alphabetLength;
        Array<typename HuffmanTree<charType>::CanonicalCode> codes;
        BitArray encodedStr;
        data_local(const uint16_t& _alphabetLength, const Array<typename HuffmanTree<charType>::CanonicalCode>& _codes, const BitArray& _encodedStr) : 
            alphabetLength(_alphabetLength), codes(_codes), encodedStr(_encodedStr) {}
        data_local() = default;
    };
    struct data {
        uint32_t inputStrSize;
        Array<data_local> localDataItems;
        data(const uint32_t _inputStrSize, const Array<data_local>& _localDataItems) : 
            inputStrSize(_inputStrSize), localDataItems(_localDataItems) {}
        data() = default;
    };

    static data encodeToData(const StringL<charType>& inputStr);
    static void encodeData(std::ofstream& outputFile, const data& data, const bool useUTF8);
    static StringL<charType> decodeData(const data& data);
};

template <typename charType>
void CodecHA<charType>::Encode(StringL<charType>& inputStr, std::ofstream& outputFile, const bool useUTF8)
{
    const size_t maxSizeOfBlock = CompressorSettings::GetHuffmanBlockSize(); 
    StringL<charType> localString(maxSizeOfBlock); 
    size_t stringPointer = 0; 
    Array<charType> alphabet;
    Array<uint32_t> frequencies;

    FileUtils::AppendValueBinary(outputFile, static_cast<uint32_t>(inputStr.size()));

    while (stringPointer < inputStr.size()) {
        localString.clear();
        while ((localString.size() < maxSizeOfBlock) && (stringPointer < inputStr.size())) {
            localString.push_back(inputStr[stringPointer++]);
        }

        alphabet = TextUtils::GetAlphabet<charType>(localString);
        frequencies = TextUtils::GetFrequenciesInt(localString, alphabet);
        sortInParallel(alphabet, frequencies);
        
        HuffmanTree<charType> tree(alphabet, frequencies);
        Array<typename HuffmanTree<charType>::CanonicalCode> huffmanCanonicalCodes = tree.GetCanonicalCodes(tree, alphabet.size());

        std::map<charType, std::pair<uint32_t, uint32_t>> huffmanCodesMap;
        for (const auto& canonicalCode : huffmanCanonicalCodes) {
            huffmanCodesMap[canonicalCode.character] = {canonicalCode.code, canonicalCode.codeLength};
        }

        BitArray encodedStr;
        uint32_t code, length;
        for (const charType& localChar : localString) {
            code = huffmanCodesMap[localChar].first;
            length = huffmanCodesMap[localChar].second;
            for (const char& bit : BinaryUtils::GetBinaryStringFromNumber(code, length)) {
                encodedStr.push_back(bit);
            }
        }

        FileUtils::AppendValueBinary(outputFile, static_cast<uint16_t>(alphabet.size()));
        Array<uint32_t> lengthsOfCodes(alphabet.size());
        if (useUTF8) {
            for (const auto& canonicalCode : huffmanCanonicalCodes) {
                CodecUTF8::EncodeCharToBinaryFile(outputFile, canonicalCode.character);
                lengthsOfCodes.push_back(canonicalCode.codeLength);
            }
        } else {
            for (const auto& canonicalCode : huffmanCanonicalCodes) {
                FileUtils::AppendValueBinary(outputFile, canonicalCode.character);
                lengthsOfCodes.push_back(canonicalCode.codeLength);
            }
        }
        encodeNumbersEffectively(outputFile, lengthsOfCodes);
        BitArray::to_file(outputFile, encodedStr);
    }
}

template <typename charType>
StringL<charType> CodecHA<charType>::Decode(std::ifstream& inputFile, const bool useUTF8)
{
    const size_t maxSizeOfBlock = CompressorSettings::GetHuffmanBlockSize();

    uint32_t inputStrSize = FileUtils::ReadValueBinary<uint32_t>(inputFile);
    uint32_t localDataCount = inputStrSize / maxSizeOfBlock + 1;
    uint32_t lastBlockSize = inputStrSize % maxSizeOfBlock;

    StringL<charType> decodedStr(localDataCount * maxSizeOfBlock);
    StringL<charType> decodedStrLocal(maxSizeOfBlock);

    while (localDataCount-- > 0) {
        uint16_t alphabetLength = FileUtils::ReadValueBinary<uint16_t>(inputFile);
        Array<charType> alphabet(alphabetLength);
        if (useUTF8) {
            for (uint16_t i = 0; i < alphabetLength; ++i)
                alphabet.push_back(CodecUTF8::DecodeCharFromBinaryFile<charType>(inputFile));
        } else {
            for (uint16_t i = 0; i < alphabetLength; ++i)
                alphabet.push_back(FileUtils::ReadValueBinary<charType>(inputFile));
        }

        Array<uint32_t> lengthsOfCodes = decodeNumbersEffectively(inputFile, alphabetLength);

        std::map<std::pair<uint32_t, uint32_t>, charType> huffmanCodesMap;
        std::pair<uint32_t, uint32_t> binRepresentation;
        std::pair<uint32_t, uint32_t> temp;
        bool isCodeUsed;
        for (size_t i = 0; i < lengthsOfCodes.size(); ++i) {
            uint32_t lengthOfCode = lengthsOfCodes[i];
            for (uint32_t j = 0; j < 1000000000; ++j) {
                binRepresentation = { j, lengthOfCode };
                if (huffmanCodesMap.find(binRepresentation) == huffmanCodesMap.end()) {
                    isCodeUsed = false;
                    temp = binRepresentation;
                    for (uint32_t _ = 0; _ < lengthOfCode; ++_) {
                        temp.first >>= 1;
                        --temp.second;
                        if (huffmanCodesMap.find(temp) != huffmanCodesMap.end()) {
                            isCodeUsed = true;
                            break;
                        }
                    }
                    if (!isCodeUsed) {
                        huffmanCodesMap[binRepresentation] = alphabet[i];
                        break;
                    }
                }
            }
        }       

        size_t localSize = (localDataCount > 0) ? maxSizeOfBlock : lastBlockSize;
        BitArray bitBuffer;
        std::pair<uint32_t, uint32_t> currentCode = { 0, 0 };
        decodedStrLocal.clear();

        size_t i = 0;
        bitBuffer = BitArray::from_file(inputFile, 8);
        while (decodedStrLocal.size() < localSize)
        {
            if (i == 8) {
                bitBuffer = BitArray::from_file(inputFile, 8);
                i = 0;
            }

            if (bitBuffer.get_bit(i++) == '0') {
                currentCode.first <<= 1;
            } else {
                currentCode.first <<= 1;
                ++currentCode.first;
            }
            ++currentCode.second;

            if (huffmanCodesMap.find(currentCode) != huffmanCodesMap.end()) {
                decodedStrLocal.push_back(huffmanCodesMap[currentCode]);
                currentCode.first = 0;
                currentCode.second = 0;
            }
        }

        decodedStr.push_back(decodedStrLocal);
    }

    return decodedStr;
}

template <typename charType>
void CodecHA<charType>::sortInParallel(Array<charType>& alphabet, Array<uint32_t>& frequencies)
{
    Array<std::pair<charType, uint32_t>> charFrequencyVector(alphabet.size());
    for (size_t i = 0; i < alphabet.size(); ++i) {
        charFrequencyVector.push_back({ alphabet[i], frequencies[i] });
    }
    std::sort(charFrequencyVector.begin(), charFrequencyVector.end(), 
        [](const std::pair<charType, uint32_t>& a, const std::pair<charType, uint32_t>& b)
        { return a.second < b.second; });
    
    alphabet.clear(); frequencies.clear();
    for (const auto& pair : charFrequencyVector) {
        alphabet.push_back(pair.first);
        frequencies.push_back(pair.second);
    }
}

template <typename charType>
void CodecHA<charType>::encodeNumbersEffectively(std::ofstream& outputFile, const Array<uint32_t>& numbers)
{
    if (numbers.size() < 1) return;

    uint32_t maxValue = 0;
    for (uint32_t number : numbers) {
        maxValue = std::max(maxValue, number);
    }

    uint32_t maxBits = std::floor(std::log2(maxValue)) + 1; 

    BitArray encoded(numbers.size() * maxBits);
    for (const uint32_t& number : numbers) {
        for (const char& bit : BinaryUtils::GetBinaryStringFromNumber(number, maxBits)) {
            encoded.push_back(bit);
        }
    }

    FileUtils::AppendValueBinary(outputFile, static_cast<uint8_t>(maxBits));
    BitArray::to_file(outputFile, encoded);
}

template <typename charType>
Array<uint32_t> CodecHA<charType>::decodeNumbersEffectively(std::ifstream& inputFile, const uint16_t numberOfElements)
{
    if (numberOfElements < 1) return Array<uint32_t>();

    uint8_t maxBits = FileUtils::ReadValueBinary<uint8_t>(inputFile);
    uint32_t encodedSize = (numberOfElements * maxBits);

    BitArray encoded = BitArray::from_file(inputFile, 
        (encodedSize % 8 == 0) ? (encodedSize) : ((encodedSize / 8 + 1) * 8)
    );

    Array<uint32_t> numbers;
    uint32_t value;
    size_t i = 0;
    while (numbers.size() < numberOfElements) {
        value = 0;
        for (uint8_t _ = 0; _ < maxBits; ++_) {
            if (encoded.get_bit(i++) == '0') {
                value <<= 1;
            } else {
                value <<= 1; value |= 1;
            }
        }
        numbers.push_back(value);
    }

    return numbers;
}

template <typename charType>
typename CodecHA<charType>::data CodecHA<charType>::encodeToData(const StringL<charType>& inputStr)
{
    Array<data_local> localDataItems;

    const size_t maxSizeOfBlock = CompressorSettings::GetHuffmanBlockSize(); 
    StringL<charType> localString(maxSizeOfBlock); 
    size_t stringPointer = 0; 
    Array<charType> alphabet;
    Array<uint32_t> frequencies; 

    while (stringPointer < inputStr.size()) {
        localString.clear();
        while ((localString.size() < maxSizeOfBlock) && (stringPointer < inputStr.size())) {
            localString.push_back(inputStr[stringPointer++]);
        }

        alphabet = TextUtils::GetAlphabet(localString);
        frequencies = TextUtils::GetFrequenciesInt(localString, alphabet);
        sortInParallel(alphabet, frequencies);

        HuffmanTree<charType> tree(alphabet, frequencies);
        Array<typename HuffmanTree<charType>::CanonicalCode> huffmanCanonicalCodes = tree.GetCanonicalCodes(tree, alphabet.size());

        std::map<charType, std::pair<uint32_t, uint32_t>> huffmanCodesMap;
        for (const auto& canonicalCode : huffmanCanonicalCodes) {
            huffmanCodesMap[canonicalCode.character] = {canonicalCode.code, canonicalCode.codeLength};
        }

        BitArray encodedStr;
        uint32_t code, length;
        for (const charType& localChar : localString) {
            code = huffmanCodesMap[localChar].first;
            length = huffmanCodesMap[localChar].second;
            for (const char& bit : BinaryUtils::GetBinaryStringFromNumber(code, length)) {
                encodedStr.push_back(bit);
            }
        }

        localDataItems.push_back(data_local(alphabet.size(), huffmanCanonicalCodes, encodedStr));
    }
    
    return data(inputStr.size(), localDataItems);
}

template <typename charType>
void CodecHA<charType>::encodeData(std::ofstream& outputFile, const data& data, const bool useUTF8)
{
    FileUtils::AppendValueBinary(outputFile, data.inputStrSize);

    for (const auto localData : data.localDataItems)
    {
        FileUtils::AppendValueBinary(outputFile, localData.alphabetLength);
        Array<uint32_t> lengthsOfCodes(localData.alphabetLength);
        if (useUTF8) {
            for (const auto& canonicalCode : localData.codes) {
                CodecUTF8::EncodeCharToBinaryFile(outputFile, canonicalCode.character);
                lengthsOfCodes.push_back(canonicalCode.codeLength);
            }
        } else {
            for (const auto& canonicalCode : localData.codes) {
                FileUtils::AppendValueBinary(outputFile, canonicalCode.character);
                lengthsOfCodes.push_back(canonicalCode.codeLength);
            }
        }
        encodeNumbersEffectively(outputFile, lengthsOfCodes);
        BitArray::to_file(outputFile, localData.encodedStr);
    }
}

template <typename charType>
StringL<charType> CodecHA<charType>::decodeData(const data& data)
{
    const size_t maxSizeOfBlock = CompressorSettings::GetHuffmanBlockSize();

    uint32_t localDataCount = data.inputStrSize / maxSizeOfBlock + 1;
    uint32_t lastBlockSize = data.inputStrSize % maxSizeOfBlock;
    StringL<charType> decodedStr(localDataCount * maxSizeOfBlock);
    StringL<charType> decodedStrLocal(maxSizeOfBlock);

    for (const auto& localData : data.localDataItems) {
        --localDataCount;
        std::map<std::pair<uint32_t, uint32_t>, charType> huffmanCodesMap;
        std::pair<uint32_t, uint32_t> binRepresentation;
        std::pair<uint32_t, uint32_t> temp;
        bool isCodeUsed;
        for (size_t i = 0; i < localData.alphabetLength; ++i) {
            uint32_t lengthOfCode = localData.codes[i].codeLength;
            for (uint32_t j = 0; j < 1000000000; ++j) {
                binRepresentation = { j, lengthOfCode };
                if (huffmanCodesMap.find(binRepresentation) == huffmanCodesMap.end()) {
                    isCodeUsed = false;
                    temp = binRepresentation;
                    for (uint32_t _ = 0; _ < lengthOfCode; ++_) {
                        temp.first >>= 1;
                        --temp.second;
                        if (huffmanCodesMap.find(temp) != huffmanCodesMap.end()) {
                            isCodeUsed = true;
                            break;
                        }
                    }
                    if (!isCodeUsed) {
                        huffmanCodesMap[binRepresentation] = localData.codes[i].character;
                        break;
                    }
                }
            }
        }   

        size_t localSize = (localDataCount > 0) ? maxSizeOfBlock : lastBlockSize;
        std::pair<uint32_t, uint32_t> currentCode = { 0, 0 };
        decodedStrLocal.clear();

        size_t i = 0;
        while (decodedStrLocal.size() < localSize)
        {          
            if (localData.encodedStr.get_bit(i++) == '0') {
                currentCode.first <<= 1;
            } else {
                currentCode.first <<= 1;
                ++currentCode.first;
            }
            ++currentCode.second;

            if (huffmanCodesMap.find(currentCode) != huffmanCodesMap.end()) {
                decodedStrLocal.push_back(huffmanCodesMap[currentCode]);
                currentCode.first = 0;
                currentCode.second = 0;
            }
        }

        decodedStr.push_back(decodedStrLocal);
    }

    return decodedStr;
}