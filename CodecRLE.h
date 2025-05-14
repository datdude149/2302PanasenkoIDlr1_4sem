#pragma once

#include <string>
#include <cstdint>

#include "../helpers/FileUtils.h"
#include "../helpers/CodecUTF8.h"
#include "../helpers/StringL.h"
#include "../helpers/Array.h"

template <typename charType>
class CodecRLE
{
public:
    static void Encode(StringL<charType>& inputStr, std::ofstream& outputFile, const bool useUTF8);
    static StringL<charType> Decode(std::ifstream& inputFile, const bool useUTF8);
private:
    CodecRLE() = default;

    static void encode(std::ofstream& outputFile, const StringL<charType>& inputStr);
    static void encode_utf8(std::ofstream& outputFile, const StringL<charType>& inputStr);
    static StringL<charType> decode(std::ifstream& inputFile);
    static StringL<charType> decode_utf8(std::ifstream& inputFile);
protected:
    struct data
    {
        uint32_t inputStrLength;
        Array<int8_t> encodedNumbers; 
        StringL<charType> encodedChars; 
        data(const uint32_t _inputStrLength, const Array<int8_t>& _encodedNumbers, const StringL<charType>& _encodedChars) : 
            inputStrLength(_inputStrLength), encodedNumbers(_encodedNumbers), encodedChars(_encodedChars) {}
        data() = default;

        StringL<charType> toString();
        static data fromString(const StringL<charType>& str);
    };

    static data encodeToData(const StringL<charType>& inputStr);
    static void encodeData(std::ofstream& outputFile, const data& data, const bool useUTF8);
    static StringL<charType> decodeData(const data& data);
};

template <typename charType>
void CodecRLE<charType>::Encode(StringL<charType>& inputStr, std::ofstream& outputFile, const bool useUTF8)
{
    if (useUTF8) encode_utf8(outputFile, inputStr);
    else encode(outputFile, inputStr);
}

template <typename charType>
StringL<charType> CodecRLE<charType>::Decode(std::ifstream& inputFile, const bool useUTF8)
{
    if (useUTF8) return decode_utf8(inputFile);
    else return decode(inputFile);
}
template <typename charType>
void CodecRLE<charType>::encode(std::ofstream& outputFile, const StringL<charType>& inputStr)
{
    FileUtils::AppendValueBinary(outputFile, static_cast<uint32_t>(inputStr.size()));

    int countIdent = 1; 
    int countUnique = 1; 
    StringL<charType> uniqueSeq; 
    bool flag = false; 
    int maxPossibleNumber = 127; 

    uniqueSeq.resize(maxPossibleNumber); 
    charType prev = inputStr[0];
    uniqueSeq.push_back(prev);

    for (size_t i = 1; i < inputStr.size(); ++i)
    {
        if (inputStr[i] == prev) 
        {
            if (countUnique > 1) {
                uniqueSeq.pop_back();
                --countUnique; 

                FileUtils::AppendValueBinary(outputFile, static_cast<int8_t>(-countUnique));
                for (size_t j = 0; j < uniqueSeq.size(); ++j)
                    FileUtils::AppendValueBinary(outputFile, uniqueSeq[j]);

                countUnique = 1;
            }

            if (flag) { countIdent = 1; flag = false; } 
            else { ++countIdent; }
            
            countUnique = 0;
            uniqueSeq.clear();
        }
        else 
        {
            if (countIdent > 1) {
                if (countIdent >= maxPossibleNumber) {
                    for (int i = 0; i < (countIdent / maxPossibleNumber); ++i) {
                        FileUtils::AppendValueBinary(outputFile, static_cast<int8_t>(maxPossibleNumber));
                        FileUtils::AppendValueBinary(outputFile, prev);
                    }
                }
                if (countIdent % maxPossibleNumber != 0) {
                    FileUtils::AppendValueBinary(outputFile, static_cast<int8_t>(countIdent % maxPossibleNumber));
                    FileUtils::AppendValueBinary(outputFile, prev);
                }
                flag = true;
                countIdent = 1;
            } else if (countIdent == 0) {
                countIdent = 1;
            }

            if (flag) {
                countUnique = 1;
                uniqueSeq.clear();
                uniqueSeq.push_back(inputStr[i]);
                flag = false;
            } else {
                if (countUnique == 0) {
                    countUnique = 1;
                    uniqueSeq.clear();
                    uniqueSeq.push_back(prev);
                }

                ++countUnique;
                uniqueSeq.push_back(inputStr[i]);
            }
            countIdent = 1;

            if (countUnique == maxPossibleNumber) {
                FileUtils::AppendValueBinary(outputFile, static_cast<int8_t>(-countUnique));
                for (size_t j = 0; j < uniqueSeq.size(); ++j) {
                    FileUtils::AppendValueBinary(outputFile, uniqueSeq[j]);                
                }
                flag = true;
                countUnique = 0;
                uniqueSeq.clear();
            }
        }
        prev = inputStr[i];
    }

    if (countIdent > 1) {
        if (countIdent >= maxPossibleNumber) {
            for (int i = 0; i < (countIdent / maxPossibleNumber); ++i) {
                FileUtils::AppendValueBinary(outputFile, static_cast<int8_t>(maxPossibleNumber));
                FileUtils::AppendValueBinary(outputFile, prev);
            }
        }
        if (countIdent % maxPossibleNumber != 0) {
            FileUtils::AppendValueBinary(outputFile, static_cast<int8_t>(countIdent % maxPossibleNumber));
            FileUtils::AppendValueBinary(outputFile, prev);
        }
    }
    if (countUnique > 0) {
        FileUtils::AppendValueBinary(outputFile, static_cast<int8_t>(-countUnique));
        for (size_t j = 0; j < uniqueSeq.size(); ++j) {
            FileUtils::AppendValueBinary(outputFile, uniqueSeq[j]);
        }
    }
    
}

template <typename charType>
void CodecRLE<charType>::encode_utf8(std::ofstream& outputFile, const StringL<charType>& inputStr)
{
    FileUtils::AppendValueBinary(outputFile, static_cast<uint32_t>(inputStr.size()));

    int countIdent = 1; 
    int countUnique = 1; 
    StringL<charType> uniqueSeq; 
    bool flag = false; 
    int maxPossibleNumber = 127; 

    uniqueSeq.resize(maxPossibleNumber); 
    charType prev = inputStr[0];
    uniqueSeq.push_back(prev);

    for (size_t i = 1; i < inputStr.size(); ++i)
    {
        if (inputStr[i] == prev) 
        {
            if (countUnique > 1) {
                uniqueSeq.pop_back(); 
                --countUnique; 
                FileUtils::AppendValueBinary(outputFile, static_cast<int8_t>(-countUnique));
                for (size_t j = 0; j < uniqueSeq.size(); ++j)
                    CodecUTF8::EncodeCharToBinaryFile(outputFile, uniqueSeq[j]);

                countUnique = 1;
            }

            if (flag) { countIdent = 1; flag = false; } 
            else { ++countIdent; }
            
            countUnique = 0;
            uniqueSeq.clear();
        }
        else 
        {
            if (countIdent > 1) {
                if (countIdent >= maxPossibleNumber) {
                    for (int i = 0; i < (countIdent / maxPossibleNumber); ++i) {
                        FileUtils::AppendValueBinary(outputFile, static_cast<int8_t>(maxPossibleNumber));
                        CodecUTF8::EncodeCharToBinaryFile(outputFile, prev);
                    }
                }
                if (countIdent % maxPossibleNumber != 0) {
                    FileUtils::AppendValueBinary(outputFile, static_cast<int8_t>(countIdent % maxPossibleNumber));
                    CodecUTF8::EncodeCharToBinaryFile(outputFile, prev);
                }
                flag = true;
                countIdent = 1;
            } else if (countIdent == 0) {
                countIdent = 1;
            }

            if (flag) {
                countUnique = 1;
                uniqueSeq.clear();
                uniqueSeq.push_back(inputStr[i]);
                flag = false;
            } else {
                if (countUnique == 0) {
                    countUnique = 1;
                    uniqueSeq.clear();
                    uniqueSeq.push_back(prev);
                }

                ++countUnique;
                uniqueSeq.push_back(inputStr[i]);
            }
            countIdent = 1;

            if (countUnique == maxPossibleNumber) {
                FileUtils::AppendValueBinary(outputFile, static_cast<int8_t>(-countUnique));
                for (size_t j = 0; j < uniqueSeq.size(); ++j) {
                    CodecUTF8::EncodeCharToBinaryFile(outputFile, uniqueSeq[j]);                
                }
                flag = true;
                countUnique = 0;
                uniqueSeq.clear();
            }
        }
        prev = inputStr[i];
    }

    if (countIdent > 1) {
        if (countIdent >= maxPossibleNumber) {
            for (int i = 0; i < (countIdent / maxPossibleNumber); ++i) {
                FileUtils::AppendValueBinary(outputFile, static_cast<int8_t>(maxPossibleNumber));
                CodecUTF8::EncodeCharToBinaryFile(outputFile, prev);
            }
        }
        if (countIdent % maxPossibleNumber != 0) {
            FileUtils::AppendValueBinary(outputFile, static_cast<int8_t>(countIdent % maxPossibleNumber));
            CodecUTF8::EncodeCharToBinaryFile(outputFile, prev);
        }
    }
    if (countUnique > 0) {
        FileUtils::AppendValueBinary(outputFile, static_cast<int8_t>(-countUnique));
        for (size_t j = 0; j < uniqueSeq.size(); ++j) {
            CodecUTF8::EncodeCharToBinaryFile(outputFile, uniqueSeq[j]);
        }
    }
    
}

template <typename charType>
StringL<charType> CodecRLE<charType>::decode(std::ifstream& inputFile)
{
    uint32_t inputStrLength = FileUtils::ReadValueBinary<uint32_t>(inputFile);

    StringL<charType> decodedStr(inputStrLength);

    uint32_t counter = 0;
    int8_t number;
    while (counter < inputStrLength)
    {
        number = FileUtils::ReadValueBinary<int8_t>(inputFile);

        if (number < 0) {
            for (int8_t i = 0; i < (-number); ++i) {
                decodedStr.push_back(FileUtils::ReadValueBinary<charType>(inputFile));
                ++counter;
            }
        } else {
            charType code = FileUtils::ReadValueBinary<charType>(inputFile);

            for (int8_t i = 0; i < number; ++i) {
                decodedStr.push_back(code);
                ++counter;
            }
        }
    }

    return decodedStr;
}

template <typename charType>
StringL<charType> CodecRLE<charType>::decode_utf8(std::ifstream& inputFile)
{
    uint32_t inputStrLength = FileUtils::ReadValueBinary<uint32_t>(inputFile);
    
    StringL<charType> decodedStr(inputStrLength);

    uint32_t counter = 0;
    int8_t number;
    while (counter < inputStrLength)
    {
        number = FileUtils::ReadValueBinary<int8_t>(inputFile);

        if (number < 0) {
            for (int8_t i = 0; i < (-number); ++i) {
                decodedStr.push_back(CodecUTF8::DecodeCharFromBinaryFile<charType>(inputFile));
                ++counter;
            }
        } else {
            charType code = CodecUTF8::DecodeCharFromBinaryFile<charType>(inputFile);

            for (int8_t i = 0; i < number; ++i) {
                decodedStr.push_back(code);
                ++counter;
            }
        }
    }

    return decodedStr;
}

template <typename charType>
typename CodecRLE<charType>::data CodecRLE<charType>::encodeToData(const StringL<charType>& inputStr)
{
    Array<int8_t> encodedNumbers;
    StringL<charType> encodedChars;

    encodedNumbers.resize(inputStr.size()); 
    encodedChars.resize(inputStr.size());

    int countIdent = 1;
    int countUnique = 1; 
    StringL<charType> uniqueSeq; 
    bool flag = false; 
    int maxPossibleNumber = 127;

    uniqueSeq.resize(maxPossibleNumber);
    charType prev = inputStr[0];
    uniqueSeq.push_back(prev);

    for (size_t i = 1; i < inputStr.size(); ++i)
    {
        if (inputStr[i] == prev) 
        {
            if (countUnique > 1) {
                uniqueSeq.pop_back(); 
                --countUnique; 

                encodedNumbers.push_back(-countUnique);
                for (size_t j = 0; j < uniqueSeq.size(); ++j)
                    encodedChars.push_back(uniqueSeq[j]);

                countUnique = 1;
            }

            if (flag) { countIdent = 1; flag = false; } 
            else { ++countIdent; }
            
            countUnique = 0;
            uniqueSeq.clear();
        }
        else 
        {
            if (countIdent > 1) {
                if (countIdent >= maxPossibleNumber) {
                    for (int i = 0; i < (countIdent / maxPossibleNumber); ++i) {
                        encodedNumbers.push_back(maxPossibleNumber);
                        encodedChars.push_back(prev);
                    }
                }
                if (countIdent % maxPossibleNumber != 0) {
                    encodedNumbers.push_back(countIdent % maxPossibleNumber);
                    encodedChars.push_back(prev);
                }
                flag = true;
                countIdent = 1;
            } else if (countIdent == 0) {
                countIdent = 1;
            }

            if (flag) {
                countUnique = 1;
                uniqueSeq.clear();
                uniqueSeq.push_back(inputStr[i]);
                flag = false;
            } else {
                if (countUnique == 0) {
                    countUnique = 1;
                    uniqueSeq.clear();
                    uniqueSeq.push_back(prev);
                }

                ++countUnique;
                uniqueSeq.push_back(inputStr[i]);
            }
            countIdent = 1;

            if (countUnique == maxPossibleNumber) {
                encodedNumbers.push_back(-countUnique);
                for (size_t j = 0; j < uniqueSeq.size(); ++j) {
                    encodedChars.push_back(uniqueSeq[j]);
                }
                flag = true;
                countUnique = 0;
                uniqueSeq.clear();
            }
        }
        prev = inputStr[i];
    }

    if (countIdent > 1) {
        if (countIdent >= maxPossibleNumber) {
            for (int i = 0; i < (countIdent / maxPossibleNumber); ++i) {
                encodedNumbers.push_back(maxPossibleNumber);
                encodedChars.push_back(prev);
            }
        }
        if (countIdent % maxPossibleNumber != 0) {
            encodedNumbers.push_back(countIdent % maxPossibleNumber);
            encodedChars.push_back(prev);
        }
    }
    if (countUnique > 0) {
        encodedNumbers.push_back(-countUnique);
        for (size_t j = 0; j < uniqueSeq.size(); ++j) {
            encodedChars.push_back(uniqueSeq[j]);
        }
    }

    return data(inputStr.size(), encodedNumbers, encodedChars);
}

template <typename charType>
void CodecRLE<charType>::encodeData(std::ofstream& outputFile, const data& data, const bool useUTF8)
{
    FileUtils::AppendValueBinary(outputFile, data.inputStrLength);

    size_t stringPointer = 0;
    if (useUTF8) {
        for (const auto& number : data.encodedNumbers) {
            FileUtils::AppendValueBinary(outputFile, number);
            if (number < 0) {
                for (int8_t i = 0; i < (-number); ++i) {
                    CodecUTF8::EncodeCharToBinaryFile(outputFile, data.encodedChars[stringPointer++]);
                }
            } else {
                CodecUTF8::EncodeCharToBinaryFile(outputFile, data.encodedChars[stringPointer++]);
            }
        }
    } else {
        for (const auto& number : data.encodedNumbers) {
            FileUtils::AppendValueBinary(outputFile, number);
            if (number < 0) {
                for (int8_t i = 0; i < (-number); ++i) {
                    FileUtils::AppendValueBinary(outputFile, data.encodedChars[stringPointer++]);
                }
            } else {
                FileUtils::AppendValueBinary(outputFile, data.encodedChars[stringPointer++]); 
            }
        }
    }
}

template <typename charType>
StringL<charType> CodecRLE<charType>::decodeData(const data& data)
{
    StringL<charType> decodedStr(data.inputStrLength);

    size_t stringPointer = 0;
    for (const auto& number : data.encodedNumbers)
    {
        if (number < 0) {
            for (int8_t i = 0; i < (-number); ++i) {
                decodedStr.push_back(data.encodedChars[stringPointer++]);
            }
        } else {
            for (int8_t i = 0; i < number; ++i) {
                decodedStr.push_back(data.encodedChars[stringPointer]); 
            }
            ++stringPointer;
        }
    }

    return decodedStr;
}


template <typename charType>
StringL<charType> CodecRLE<charType>::data::toString()
{
    StringL<charType> result(inputStrLength);

    size_t stringPointer = 0;
    for (const int8_t& number : encodedNumbers)
    {
        result.push_back(static_cast<charType>(number + 128)); 
        
        if (number >= 0) {
            result.push_back(encodedChars[stringPointer++]); 
        } else {
            for (int8_t _ = number; _ < 0; ++_) {
                result.push_back(encodedChars[stringPointer++]);
            }
        }
    }

    return result;
}

template <typename charType>
typename CodecRLE<charType>::data CodecRLE<charType>::data::fromString(const StringL<charType>& str)
{
    uint32_t encodedNumbersLength = 0, encodedCharsLength = 0, inputStrLength = 0;
    int8_t number;

    size_t i = 0;
    while (i < str.size()) {
        number = str[i++] - 128; 
        ++encodedNumbersLength;

        if (number >= 0) {
            ++i; ++encodedCharsLength;
            inputStrLength += number;
        } else {
            i += -number;
            encodedCharsLength += -number;
            inputStrLength += -number;
        }
    }

    Array<int8_t> encodedNumbers(encodedNumbersLength);
    StringL<charType> encodedChars(encodedCharsLength);

    i = 0;
    while (i < str.size()) {
        number = str[i++] - 128;
        encodedNumbers.push_back(number);

        if (number >= 0) {
            encodedChars.push_back(str[i++]);
        } else {
            for (int8_t _ = number; _ < 0; ++_) {
                encodedChars.push_back(str[i++]);
            }
        }
    }

    return data(inputStrLength, encodedNumbers, encodedChars);
}