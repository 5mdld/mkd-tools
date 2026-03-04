//
// kiwakiwaaにより 2026/02/27 に作成されました。
//

#pragma once

#include "binary_file_reader.hpp"

namespace MKD
{
    class ReadSequence
    {
    public:
        explicit ReadSequence(BinaryFileReader& reader);

        template<typename T>
        T read()
        {
            if (failed_) return T{};

            auto result = reader_.readStruct<T>();
            if (!result)
            {
                captureError(result.error());
                return T{};
            }
            return std::move(*result);
        }

        template<std::integral T>
        T readValue()
        {
            if (failed_) return T{};

            auto result = reader_.read<T>();
            if (!result)
            {
                captureError(result.error());
                return T{};
            }
            return *result;
        }

        template<typename T>
        std::vector<T> readArray(const size_t count)
        {
            if (failed_) return std::vector<T>{};

            auto result = reader_.readStructArray<T>(count);
            if (!result)
            {
                captureError(result.error());
                return std::vector<T>{};
            }
            return std::move(*result);
        }

        std::vector<uint8_t> readBytes(size_t count);

        std::string readString(size_t size);

        [[nodiscard]] size_t remaining() const;

        void seek(size_t offset);

        [[nodiscard]] explicit operator bool() const;
        [[nodiscard]] const std::string& error() const;

    private:
        void captureError(const std::string& err);

        BinaryFileReader& reader_;
        bool failed_ = false;
        std::string error_;
    };


    inline ReadSequence BinaryFileReader::sequence() { return ReadSequence{*this}; }
}