//
// kiwakiwaaにより 2026/10/24 に作成されました。
//

#pragma once

#include "MKD/dictionary/authoring_diagnostics.hpp"
#include "MKD/dictionary/metadata.hpp"
#include <string_view>

namespace MKD::detail
{
    AuthoringOrigin origin(std::string operation, std::string subject, SourceContext context = {});

    struct DependencyOrigins
    {
        size_t occurrences = 0;
        std::vector<AuthoringOrigin> samples;
        void add(AuthoringOrigin value);
    };

    class IssueCollector
    {
    public:
        explicit IssueCollector(const ValidationOptions options) : options_(options)
        {
        }

        void add(AuthoringIssue issue);
        void add(std::string code, std::string subject, std::string message, AuthoringOrigin source);
        void add(std::string code, const DictionaryContent& dictionary, std::string subject, std::string message,
                 const DependencyOrigins& origins = {});
        void pending(std::string dictionary);
        [[nodiscard]] ValidationReport take() &&;

    private:
        ValidationOptions options_;
        ValidationReport report_;
    };
}
