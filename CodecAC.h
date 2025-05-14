#pragma once

#include <cstdint>
#include <map>
#include <cmath>

#include "../helpers/FileUtils.h"
#include "../helpers/CodecUTF8.h"
#include "../helpers/TextUtils.h"
#include "../helpers/BinaryUtils.h"
#include "../helpers/BitArray.h"
#include "../helpers/StringL.h"
#include "../helpers/Array.h"

template <typename charType>
class CodecAC
{
public:
    static void Encode(StringL<charType>& inputStr, std::ofstream& outputFile, const bool useUTF8);
    static StringL<charType> Decode(std::ifstream& inputFile, const bool useUTF8);
private:
    CodecAC() = default;

    static inline void backSortInParallel(Array<charType>& alphabet, Array<double>& frequencies);
    static Array<double> calculateSegments(const Array<charType>& alphabet, const Array<double>& frequencies);
    static inline void binSearchStep(const Array<double>& segments, const double value, const char directionBit, int& start, int& end);
    static inline bool foundLetter(const Array<double>& segments, const size_t index, const double low, const double high);
    static void encodeFrequencies(std::ofstream& outputFile, const uint32_t strLength, const Array<double>& frequencies);
    static Array<double> decodeFrequencies(std::ifstream& inputFile, const uint32_t strLength, const uint32_t alphabetLength);
protected:
    struct data {
        uint32_t inputStrLength;
        uint32_t alphabetLength;
        Array<charType> alphabet;
        Array<double> frequencies;
        BitArray resultValue;
        data(const uint32_t& _inputStrLength, const uint32_t& _alphabetLength, const Array<charType>& _alphabet, const Array<double>& _frequencies, const BitArray& _resultValue) : 
            inputStrLength(_inputStrLength), alphabetLength(_alphabetLength), alphabet(_alphabet), frequencies(_frequencies), resultValue(_resultValue) {}
        data() = default;
    };

    static data encodeToData(const StringL<charType>& inputStr);
    static void encodeData(std::ofstream& outputFile, const data& data, const bool useUTF8);
    static StringL<charType> decodeData(const data& data);
};

template <typename charType>
void CodecAC<charType>::Encode(StringL<charType>& inputStr, std::ofstream& outputFile, const bool useUTF8)
{
    if (inputStr.size() < 1) {
        FileUtils::AppendValueBinary(outputFile, static_cast<uint32_t>(0));
        FileUtils::AppendValueBinary(outputFile, static_cast<uint32_t>(0));
    }

    double low, high, mid;
    double low_temp, high_temp, mid_temp;
    BitArray encodedBits;

    Array<charType> alphabet = TextUtils::GetAlphabet(inputStr);
    Array<double> frequencies = TextUtils::GetFrequencies(inputStr, alphabet);
    backSortInParallel(alphabet, frequencies);
    Array<double> segments = calculateSegments(alphabet, frequencies);

    int k = 1, temp = alphabet.size();
    while (temp > 1) { temp /= 2; ++k; }
    encodedBits.resize(inputStr.size() * k);

    size_t index;

    for (const charType& c : inputStr)
	{
        for (index = 0; index < alphabet.size(); ++index) {
            if (c == alphabet[index]) break;
        }

        high_temp = 1.0;
        low_temp = 0.0;
        mid_temp = (high_temp + low_temp) / 2.0;

        low = segments[index];
		high = segments[index + 1];
        mid = (low + high) / 2.0;

		while (!((high_temp <= high) && (low_temp >= low)))
		{
			if (mid_temp > mid) {
                encodedBits.push_back('0');
                high_temp = mid_temp;
			} else {
                encodedBits.push_back('1');
				low_temp = mid_temp;
			}

            mid_temp = (high_temp + low_temp) / 2.0;
		}
	}

    FileUtils::AppendValueBinary(outputFile, static_cast<uint32_t>(inputStr.size()));
    FileUtils::AppendValueBinary(outputFile, static_cast<uint32_t>(alphabet.size()));
    if (useUTF8) {
        for (const charType& c : alphabet)
            CodecUTF8::EncodeCharToBinaryFile(outputFile, c);
    } else {
        for (const charType& c : alphabet)
            FileUtils::AppendValueBinary(outputFile, c);
    }
    encodeFrequencies(outputFile, inputStr.size(), frequencies);
    BitArray::to_file(outputFile, encodedBits);
}

template <typename charType>
StringL<charType> CodecAC<charType>::Decode(std::ifstream& inputFile, const bool useUTF8)
{
    uint32_t inputStrLength = FileUtils::ReadValueBinary<uint32_t>(inputFile);
    if (inputStrLength < 1) { return StringL<charType>(); }

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
    Array<double> frequencies = decodeFrequencies(inputFile, inputStrLength, alphabet.size());
    Array<double> segments = calculateSegments(alphabet, frequencies);

    double low, high, mid;
    int segments_lowInd, segments_highInd;
    size_t i = 0;
    StringL<char> encodedBitsBlock = BinaryUtils::GetBinaryStringFromNumber<uint8_t>(
        FileUtils::ReadValueBinary<uint8_t>(inputFile), 8);

    StringL<charType> decodedStr(inputStrLength);

    while (decodedStr.size() < inputStrLength) {
        low = 0.0; high = 1.0;
        mid = (high + low) / 2.0;

        segments_lowInd = 0;
        segments_highInd = segments.size() - 1;

        while (true)
        {
            if (i == 8) {
                encodedBitsBlock = BinaryUtils::GetBinaryStringFromNumber<uint8_t>(
                    FileUtils::ReadValueBinary<uint8_t>(inputFile), 8);
                i = 0;
            }

            if (encodedBitsBlock[i] == '0') {
                high = mid;
            } else {
                low = mid;
            }

            binSearchStep(segments, mid, encodedBitsBlock[i], segments_lowInd, segments_highInd);
            
            mid = (high + low) / 2.0;
            ++i;

            if (segments_lowInd >= segments_highInd - 1) {
                if (foundLetter(segments, segments_lowInd, low, high)) {
                    break;
                }
            }
        }


        decodedStr.push_back(alphabet[segments_lowInd]);
    }
    
    return decodedStr;
}
template <typename charType>
void CodecAC<charType>::backSortInParallel(Array<charType>& alphabet, Array<double>& frequencies)
{
    Array<std::pair<charType, double>> charFrequencyVector;
    for (size_t i = 0; i < alphabet.size(); ++i) {
        charFrequencyVector.push_back({ alphabet[i], frequencies[i] });
    }
    std::sort(charFrequencyVector.begin(), charFrequencyVector.end(), 
        [](const std::pair<charType, double>& a, const std::pair<charType, double>& b)
        { return a.second > b.second; });
    
    alphabet.clear(); frequencies.clear();
    for (const auto& pair : charFrequencyVector) {
        alphabet.push_back(pair.first);
        frequencies.push_back(pair.second);
    }
}

template <typename charType>
Array<double> CodecAC<charType>::calculateSegments(const Array<charType>& alphabet, const Array<double>& frequencies)
{
    Array<double> segments;
    if (alphabet.size() < 2) {
        segments = { 0.0, 0.5, 1.0 };
    } else {
        segments.resize(alphabet.size() + 1);
        segments.push_back(0.0);
        for (size_t i = 1; i < alphabet.size(); ++i) {            
            segments.push_back(frequencies[i - 1] + segments[i - 1]);
        }
        segments.push_back(1.0);
    }

    return segments;
}

template <typename charType>
void CodecAC<charType>::binSearchStep(const Array<double>& segments, const double value, const char directionBit, int& start, int& end)
{
    if (!((segments[start] <= value) && (value <= segments[end]))) {
        throw std::runtime_error("CodecAC error: value is not in [segments[start], segments[end]]");
    }

    int left = start, right = end;
    int center = (left + right) / 2;
    
    while (true) {
        if (value >= segments[center])
            left = center;
        else
            right = center;
        
        center = (left + right) / 2;
        if (left >= right - 1){
            if (directionBit == '0') {
                end = right;
            } else {
                start = left;
            }
            return;
        }
    }
}

template <typename charType>
bool CodecAC<charType>::foundLetter(const Array<double>& segments, const size_t index, const double low, const double high)
{
    return (low >= segments[index]) && (high <= segments[index + 1]);
}

template <typename charType>
void CodecAC<charType>::encodeFrequencies(std::ofstream& outputFile, const uint32_t strLength, const Array<double>& frequencies)
{
    if (strLength < 1) {
        return;
    }

    Array<uint32_t> frequenciesInt(frequencies.size()); 
    for (const double& freq : frequencies) {
        frequenciesInt.push_back(std::round(freq * strLength));
    }
    
    uint32_t maxValue = frequenciesInt[0]; 
    int maxBits = std::floor(std::log2(maxValue)) + 1; 

    BitArray encoded(frequenciesInt.size() * maxBits);
    for (const uint32_t& freqInt : frequenciesInt) {
        for (const char& bit : BinaryUtils::GetBinaryStringFromNumber(freqInt, maxBits)) {
            encoded.push_back(bit);
        }
    }

    FileUtils::AppendValueBinary(outputFile, static_cast<uint8_t>(maxBits));
    BitArray::to_file(outputFile, encoded);
}

template <typename charType>
Array<double> CodecAC<charType>::decodeFrequencies(std::ifstream& inputFile, const uint32_t strLength, const uint32_t alphabetLength)
{
    if (strLength < 1) {
        return Array<double>();
    }

    uint8_t maxBits = FileUtils::ReadValueBinary<uint8_t>(inputFile);
    uint32_t encodedSize = (alphabetLength * maxBits);
    while (encodedSize % 8 != 0) ++encodedSize; 

    BitArray encoded = BitArray::from_file(inputFile, encodedSize);
    

    Array<double> frequencies(alphabetLength);
    size_t i = 0;
    uint32_t value;
    while (frequencies.size() < alphabetLength) {
        value = 0;
        for (uint8_t _ = 0; _ < maxBits; ++_) {
            if (encoded.get_bit(i++) == '0') {
                value <<= 1;
            } else {
                value <<= 1; value |= 1;
            }
        }
        frequencies.push_back( value / static_cast<double>(strLength) );
    }

    return frequencies;
}

template <typename charType>
typename CodecAC<charType>::data CodecAC<charType>::encodeToData(const StringL<charType>& inputStr)
{
    if (inputStr.size() < 1) {
        return data(0, 0, Array<charType>(), Array<double>(), BitArray());
    }

    double low, high, mid;
    double low_temp, high_temp, mid_temp;
    BitArray encodedBits;

    Array<charType> alphabet = TextUtils::GetAlphabet<charType>(inputStr);
    Array<double> frequencies = TextUtils::GetFrequencies(inputStr, alphabet);
    backSortInParallel(alphabet, frequencies);
    Array<double> segments = calculateSegments(alphabet, frequencies);

    int k = 1, temp = alphabet.size();
    while (temp > 1) { temp /= 2; ++k; }
    encodedBits.resize(inputStr.size() * k);

    size_t index;

    for (const charType& c : inputStr)
	{
        for (index = 0; index < alphabet.size(); ++index) {
            if (c == alphabet[index]) break;
        }

        high_temp = 1.0;
        low_temp = 0.0;
        mid_temp = (high_temp + low_temp) / 2.0;

        low = segments[index];
		high = segments[index + 1];
        mid = (low + high) / 2.0;

		while (!((high_temp <= high) && (low_temp >= low)))
		{
			if (mid_temp > mid) {
                encodedBits.push_back('0');
                high_temp = mid_temp;
			} else {
                encodedBits.push_back('1');
				low_temp = mid_temp;
			}

            mid_temp = (high_temp + low_temp) / 2.0;
		}
	}

    return data(inputStr.size(), alphabet.size(), alphabet, frequencies, encodedBits);
}

template <typename charType>
void CodecAC<charType>::encodeData(std::ofstream& outputFile, const data& data, const bool useUTF8)
{
    FileUtils::AppendValueBinary(outputFile, data.inputStrLength);
    FileUtils::AppendValueBinary(outputFile, data.alphabetLength);
    if (useUTF8) {
        for (const charType& c : data.alphabet)
            CodecUTF8::EncodeCharToBinaryFile(outputFile, c);
    } else {
        for (const charType& c : data.alphabet)
            FileUtils::AppendValueBinary(outputFile, c);
    }
    encodeFrequencies(outputFile, data.inputStrLength, data.frequencies);
    BitArray::to_file(outputFile, data.resultValue);
}

template <typename charType>
StringL<charType> CodecAC<charType>::decodeData(const data& data)
{
    if (data.inputStrLength < 1) {
        return StringL<charType>();
    }

    StringL<charType> decodedStr(data.inputStrLength);
    
    Array<double> segments = calculateSegments(data.alphabet, data.frequencies);
    double low, high, mid;
    int segments_lowInd, segments_highInd;
    char bit;
    size_t i = 0;

    while (decodedStr.size() < data.inputStrLength) {
        low = 0.0; high = 1.0;
        mid = (high + low) / 2.0;

        segments_lowInd = 0;
        segments_highInd = segments.size() - 1;

        while (true)
        {
            bit = data.resultValue.get_bit(i);
            if (bit == '0') {
                high = mid;
            } else {
                low = mid;
            }

            binSearchStep(segments, mid, bit, segments_lowInd, segments_highInd);
            
            mid = (high + low) / 2.0;
            ++i;

            if (segments_lowInd >= segments_highInd - 1) {
                if (foundLetter(segments, segments_lowInd, low, high)) {
                    break;
                }
            }
        }

        decodedStr.push_back(data.alphabet[segments_lowInd]);
    }

    return decodedStr;
}