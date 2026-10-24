//
// kiwakiwaaにより 2026/10/19 に作成されました。
//

#include "dictionary/navigation/native_navigation.hpp"

namespace MKD::detail::native::navigation
{
    Result<Header> Header::read(BinaryView data)
    {
        if (auto header = data.slice(0, headerSize); !header)
            return std::unexpected(header.error());

        Header result{
            *data.readLittle<uint8_t>(0), static_cast<ItemKind>(*data.readLittle<uint8_t>(1)),
            *data.readLittle<uint16_t>(2), *data.readLittle<uint32_t>(4),
            *data.readLittle<uint32_t>(8), *data.readLittle<uint32_t>(12)
        };
        const auto kind = static_cast<size_t>(result.itemKind);
        if (result.titleKind > 1 || kind >= strides.size())
            return std::unexpected("Unknown navigation record type");

        if ((result.flags & sections) && result.itemKind != ItemKind::Section)
            return std::unexpected("Navigation sections require child headers");

        if (result.count && result.data < headerSize)
            return std::unexpected("Navigation records overlap their header");

        if (auto table = data.table(result.data, result.count, strides[kind]); !table)
            return std::unexpected(table.error());

        return result;
    }

    Result<std::string> text(BinaryView data, const size_t offset)
    {
        if (!offset)
            return std::string{};

        auto length = data.readLittle<uint32_t>(offset);
        if (!length)
            return std::unexpected(length.error());

        auto bytes = data.slice(offset + 4, *length);
        if (!bytes)
            return std::unexpected(bytes.error());

        if (*length % 2)
            return std::unexpected("Odd navigation UTF-16 length");

        std::u16string decoded(*length / 2, u'\0');
        for (size_t i = 0; i < decoded.size(); ++i)
            decoded[i] = *bytes->readLittle<uint16_t>(i * 2);

        if (unicode::firstInvalidUtf16Offset(decoded))
            return std::unexpected("Invalid navigation UTF-16");

        return unicode::toUtf8(decoded);
    }

    Result<AppendixEntryLink> entry(BinaryView data, const size_t offset)
    {
        auto record = data.slice(offset, 8);
        if (!record)
            return std::unexpected(record.error());

        return AppendixEntryLink{
            {*record->readLittle<uint32_t>(0), *record->readLittle<uint16_t>(4)},
            *record->readLittle<uint16_t>(6)
        };
    }

    std::string encodeLabel(const NavigationLabel& label)
    {
        if (const auto* text = std::get_if<std::string>(&label))
            return *text;

        if (const auto* radical = std::get_if<RadicalLabel>(&label))
            return "／1／" + radical->character + "／" + radical->variants + "／" + radical->reading;

        return {};
    }

    NavigationLabel decodeLabel(std::string value)
    {
        constexpr std::string_view prefix = "／1／", separator = "／";
        if (value.starts_with(prefix))
        {
            const auto first = value.find(separator, prefix.size());
            const auto second = first == std::string::npos ? first : value.find(separator, first + separator.size());
            if (second != std::string::npos)
                return RadicalLabel{
                    value.substr(prefix.size(), first - prefix.size()),
                    value.substr(first + separator.size(), second - first - separator.size()),
                    value.substr(second + separator.size())
                };
        }
        return value;
    }

    std::string plainLabel(const NavigationLabel& label)
    {
        if (const auto* radical = std::get_if<RadicalLabel>(&label))
            return radical->character;

        return encodeLabel(label);
    }
}
