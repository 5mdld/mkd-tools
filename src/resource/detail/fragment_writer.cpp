//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#include "fragment_writer.hpp"

namespace MKD::detail
{
    FragmentWriter::FragmentWriter(std::unique_ptr<BuildDirectory> build, SegmentedFileWriter files, std::string prefix)
        : build_(std::move(build)), files_(std::move(files)), prefix_(std::move(prefix))
    {
    }

    Result<FragmentWriter> FragmentWriter::create(const std::filesystem::path& directory,
                                                  const ResourceType type, const uint32_t fileBytes)
    {
        if (type != ResourceType::Fonts && type != ResourceType::Contents && type != ResourceType::AudioLegacy)
            return std::unexpected("Fragment store requires an RSC resource type");

        auto build = BuildDirectory::create(directory);
        if (!build)
            return std::unexpected(build.error());

        auto files = SegmentedFileWriter::create((*build)->path(), type, fileBytes,
                                                 SegmentedFileWriter::lastRscSequence);
        if (!files)
            return std::unexpected(files.error());

        auto prefix = resourceTraits(type).blobPrefix;
        if (prefix.ends_with('-'))
            prefix.remove_suffix(1);

        return FragmentWriter(std::move(*build), std::move(*files), std::string(prefix));
    }

    Result<void> FragmentWriter::add(std::span<const uint8_t> bytes)
    {
        if (auto state = build_->check(); !state)
            return state;

        if (bytes.size() > UINT32_MAX - 4 || files_.offset() > UINT32_MAX - 4 - bytes.size() || count_ == UINT32_MAX)
            return build_->fail("Fragment store exceeds format limits");

        BinaryBuffer header(4);
        header.writeLittle(static_cast<uint32_t>(bytes.size()));

        auto location = files_.write({header.data(), bytes});
        if (!location)
            return build_->fail(location.error());

        records_.writeLittle(static_cast<uint32_t>(location->globalOffset));
        records_.writeLittle(UINT32_MAX);
        ++count_;
        return {};
    }

    Result<void> FragmentWriter::finish()
    {
        if (auto state = build_->check(); !state)
            return state;

        if (auto closed = files_.finish(); !closed)
            return build_->fail(closed.error());

        BinaryBuffer map(8 + records_.position());
        map.writeLittle(uint32_t{0});
        map.writeLittle(count_);
        map.writeBytes(records_.data());

        if (auto written = OutputFile::writeAll(build_->path() / (prefix_ + ".map"), map.data()); !written)
            return build_->fail(written.error());

        return build_->publish();
    }
}
