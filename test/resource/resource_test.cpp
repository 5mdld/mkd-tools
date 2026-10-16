//
// kiwakiwaaにより 2026/10/16 に作成されました。
//

#include <gtest/gtest.h>

#include "MKD/mkd.hpp"
#include "binary/binary_view.hpp"
#include "binary/binary_buffer.hpp"
#include "resource/rsc/resource_store_index.hpp"
#include "resource/rsc/resource_store_writer.hpp"
#include "resource/nrsc/named_resource_store_index.hpp"
#include "resource/nrsc/named_resource_store_writer.hpp"
#include "resource/keystore/keystore_writer.hpp"

#include <array>
#include <chrono>
#include <fstream>
#include <limits>
#include <random>
#include <sstream>
#include <unordered_set>

namespace MKD::test
{
    class ResourceFormatTest : public ::testing::Test
    {
    protected:
        std::filesystem::path directory;

        void SetUp() override
        {
            directory = std::filesystem::temp_directory_path() /
                ("mkd-resources-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) +
                 "-" + std::to_string(std::random_device{}()));
            std::filesystem::create_directories(directory);
        }

        void TearDown() override
        {
            std::error_code ec;
            std::filesystem::remove_all(directory, ec);
        }

        void write(std::string_view name, std::span<const uint8_t> bytes)
        {
            std::ofstream file(directory / name, std::ios::binary);
            file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
            ASSERT_TRUE(file);
        }

        void words(std::string_view name, std::initializer_list<uint32_t> values)
        {
            detail::BinaryBuffer bytes;
            for (auto value : values)
                bytes.writeLittle(value);
            write(name, bytes.data());
        }

        void directStore()
        {
            words("contents.map", {0, 2, 0, 0xFFFFFFFF, 5, 0xFFFFFFFF});
            words("contents.idx", {2, 0, 10, 1, 20, 0});
            const std::array<uint8_t, 10> data{1, 0, 0, 0, 'a', 1, 0, 0, 0, 'b'};
            write("contents-0001.rsc", data);
        }
    };

    TEST_F(ResourceFormatTest, ReadsUnalignedIntegersInBothByteOrders)
    {
        const std::array<uint8_t, 9> bytes{0, 1, 2, 3, 4, 5, 6, 7, 8};
        const detail::BinaryView view(bytes);
        EXPECT_EQ(*view.readLittle<uint16_t>(1), 0x0201);
        EXPECT_EQ(*view.readBig<uint16_t>(1), 0x0102);
        EXPECT_EQ(*view.readLittle<uint32_t>(1), 0x04030201u);
        EXPECT_EQ(*view.readBig<uint32_t>(1), 0x01020304u);
        EXPECT_EQ((*view.readBig<uint32_t, 3>(1)), 0x010203u);
        EXPECT_EQ((*view.readLittle<uint32_t, 3>(1)), 0x030201u);
        EXPECT_EQ(*view.readBig<uint64_t>(1), 0x0102030405060708ull);
        EXPECT_EQ(*view.readLittle<uint64_t>(1), 0x0807060504030201ull);
    }

    TEST_F(ResourceFormatTest, ChecksRangesAndTableArithmeticBeforeReading)
    {
        const std::array<uint8_t, 4> bytes{1, 2, 3, 4};
        const detail::BinaryView view(bytes);
        EXPECT_FALSE(view.readLittle<uint32_t>(1));
        EXPECT_FALSE(view.slice(std::numeric_limits<size_t>::max(), 2));
        EXPECT_FALSE(view.slice(1, std::numeric_limits<size_t>::max()));
        EXPECT_FALSE(view.table(0, std::numeric_limits<size_t>::max(), 8));
        EXPECT_FALSE(view.table(0, 1, 0));
        EXPECT_TRUE(view.slice(4, 0));
        EXPECT_FALSE(view.nullTerminatedString(0));
        EXPECT_FALSE(view.tail(5));
        write("bytes", bytes);
        auto mapping = MappedFile::open(directory / "bytes");
        ASSERT_TRUE(mapping);
        EXPECT_FALSE(mapping->slice(std::numeric_limits<size_t>::max(), 2));
    }

    TEST_F(ResourceFormatTest, SerialisesExplicitByteOrdersWithoutStructLayout)
    {
        detail::BinaryBuffer bytes;
        bytes.writeLittle(uint16_t{0x1234});
        bytes.writeBig(uint32_t{0x12345678});
        bytes.writeBig<uint32_t, 3>(0xabcdef);
        const std::vector<uint8_t> expected{0x34, 0x12, 0x12, 0x34, 0x56, 0x78, 0xab, 0xcd, 0xef};
        EXPECT_EQ(bytes.take(), expected);
        EXPECT_TRUE(bytes.data().empty());
        bytes.writeByte(42);
        EXPECT_EQ(bytes.take(), std::vector<uint8_t>{42});
    }

}
