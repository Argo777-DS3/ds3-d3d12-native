#include <immintrin.h>

#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

using Bits4 = std::array<std::uint32_t, 4>;

std::vector<std::string> Split(const std::string& line) {
    std::vector<std::string> fields;
    std::stringstream stream(line);
    std::string field;
    while (std::getline(stream, field, ',')) fields.push_back(field);
    if (!line.empty() && line.back() == ',') fields.emplace_back();
    return fields;
}

std::uint64_t ParseUnsigned(const std::string& text) {
    size_t consumed = 0;
    const int base = text.size() > 2 && text[0] == '0' &&
                             (text[1] == 'x' || text[1] == 'X')
                         ? 16
                         : 10;
    const std::uint64_t value = std::stoull(text, &consumed, base);
    if (consumed != text.size()) throw std::runtime_error("invalid integer: " + text);
    return value;
}

std::string Hex32(std::uint32_t value) {
    char text[16]{};
    std::snprintf(text, sizeof(text), "0x%08X", value);
    return text;
}

std::string JsonEscape(const std::string& value) {
    std::string result;
    for (char character : value) {
        if (character == '\\' || character == '"') result.push_back('\\');
        result.push_back(character);
    }
    return result;
}

struct RowView {
    const std::vector<std::string>& fields;
    const std::unordered_map<std::string, size_t>& columns;

    const std::string& Get(const char* name) const {
        const auto found = columns.find(name);
        if (found == columns.end() || found->second >= fields.size()) {
            throw std::runtime_error(std::string("missing column: ") + name);
        }
        return fields[found->second];
    }

    std::uint64_t U64(const char* name) const { return ParseUnsigned(Get(name)); }

    Bits4 Bits(const char* prefix) const {
        Bits4 values{};
        for (size_t lane = 0; lane < values.size(); ++lane) {
            const std::string name = std::string(prefix) + std::to_string(lane);
            values[lane] = static_cast<std::uint32_t>(U64(name.c_str()));
        }
        return values;
    }
};

Bits4 StoreBits(__m128 value) {
    Bits4 result{};
    _mm_storeu_si128(reinterpret_cast<__m128i*>(result.data()), _mm_castps_si128(value));
    return result;
}

__m128 LoadBits(const Bits4& value) {
    return _mm_castsi128_ps(
        _mm_loadu_si128(reinterpret_cast<const __m128i*>(value.data())));
}

void AppendIssue(std::string& issues, const char* issue) {
    if (!issues.empty()) issues.push_back(';');
    issues += issue;
}

void WriteBits(std::ofstream& output, const Bits4& bits) {
    for (std::uint32_t value : bits) output << ',' << Hex32(value);
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "Usage: reconstruct_writer_mirror <input.csv> <output.csv> <summary.json>\n";
        return 2;
    }

    try {
        std::ifstream input(argv[1]);
        if (!input) throw std::runtime_error("cannot open input CSV");
        std::ofstream output(argv[2]);
        if (!output) throw std::runtime_error("cannot open output CSV");

        std::string line;
        if (!std::getline(input, line)) throw std::runtime_error("empty input CSV");
        const std::vector<std::string> header = Split(line);
        std::unordered_map<std::string, size_t> columns;
        for (size_t index = 0; index < header.size(); ++index) {
            columns.emplace(header[index], index);
        }

        output << "seq,trial,broad_update_index,writer_invocation_index,source,mxcsr,"
                  "qualified_transform,will_commit,precommit_valid,issues";
        for (const char* prefix : {"p_b", "d_b", "b_b", "x_reconstructed_b", "a_b"}) {
            for (unsigned int lane = 0; lane < 4; ++lane) {
                output << ',' << prefix << lane;
            }
        }
        output << '\n';

        std::uint64_t rows = 0;
        std::uint64_t qualified_rows = 0;
        std::uint64_t valid_rows = 0;
        std::unordered_map<std::string, std::uint64_t> issue_counts;
        const unsigned int original_mxcsr = _mm_getcsr();

        while (std::getline(input, line)) {
            if (line.empty()) continue;
            const std::vector<std::string> fields = Split(line);
            const RowView row{fields, columns};
            ++rows;

            const Bits4 current_memory = row.Bits("current_mem_b");
            const Bits4 current_xmm1 = row.Bits("current_xmm1_b");
            const Bits4 raw_candidate = row.Bits("raw_candidate_b");
            const Bits4 effective_candidate = row.Bits("effective_candidate_b");
            const Bits4 scalar_xmm0 = row.Bits("scalar_xmm0_b");
            const Bits4 final_xmm6 = row.Bits("final_xmm6_b");
            const std::uint32_t scalar_memory =
                static_cast<std::uint32_t>(row.U64("scalar_memory_bits"));
            const unsigned int mxcsr = static_cast<unsigned int>(row.U64("mxcsr"));

            _mm_setcsr(mxcsr);
            const Bits4 mask_bits = {0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0u};
            const Bits4 homogeneous_bits = {0u, 0u, 0u, 0x3F800000u};
            const __m128 q = _mm_and_ps(LoadBits(raw_candidate), LoadBits(mask_bits));
            const __m128 p_value = _mm_add_ps(q, LoadBits(homogeneous_bits));
            const __m128 d_value = _mm_sub_ps(p_value, LoadBits(current_xmm1));
            const __m128 b_value = _mm_mul_ps(d_value, LoadBits(scalar_xmm0));
            const __m128 x_value = _mm_add_ps(b_value, LoadBits(current_xmm1));
            const __m128 a_value = _mm_sub_ps(LoadBits(final_xmm6),
                                               LoadBits(current_xmm1));
            const Bits4 p = StoreBits(p_value);
            const Bits4 d = StoreBits(d_value);
            const Bits4 b = StoreBits(b_value);
            const Bits4 reconstructed = StoreBits(x_value);
            const Bits4 applied = StoreBits(a_value);
            _mm_setcsr(original_mxcsr);

            std::string issues;
            const bool qualified = row.U64("rbx_matches_transform") == 1;
            if (qualified) ++qualified_rows;
            const auto require_one = [&](const char* name, const char* issue) {
                if (row.U64(name) != 1) AppendIssue(issues, issue);
            };
            if (qualified) {
                require_one("current_memory_valid", "current_memory_invalid");
                require_one("raw_candidate_valid", "raw_candidate_invalid");
                require_one("outer_a8_read_valid", "outer_a8_invalid");
                require_one("outer_b0_read_valid", "outer_b0_invalid");
                require_one("nested_read_valid", "nested_invalid");
                require_one("effective_candidate_valid", "effective_candidate_invalid");
                require_one("scalar_memory_valid", "scalar_memory_invalid");
                require_one("inhibit_valid", "inhibit_invalid");
                if (row.U64("overflow") != 0) AppendIssue(issues, "overflow");
                if (row.Get("source") == "NONE") AppendIssue(issues, "source_none");
                if (current_memory != current_xmm1) AppendIssue(issues, "current_mismatch");
                if (raw_candidate != effective_candidate) AppendIssue(issues, "candidate_mismatch");
                for (std::uint32_t lane : scalar_xmm0) {
                    if (lane != scalar_memory) {
                        AppendIssue(issues, "scalar_mismatch");
                        break;
                    }
                }
                if (reconstructed != final_xmm6) {
                    AppendIssue(issues, "reconstruction_mismatch");
                }
            }

            const bool valid = qualified && issues.empty();
            if (valid) ++valid_rows;
            if (qualified) {
                std::stringstream issue_stream(issues);
                std::string issue;
                while (std::getline(issue_stream, issue, ';')) {
                    if (!issue.empty()) ++issue_counts[issue];
                }
            }

            output << row.Get("seq") << ',' << row.Get("trial") << ','
                   << row.Get("broad_update_index") << ','
                   << row.Get("writer_invocation_index") << ','
                   << row.Get("source") << ',' << row.Get("mxcsr") << ','
                   << (qualified ? 1 : 0) << ',' << row.Get("will_commit") << ','
                   << (valid ? 1 : 0) << ',' << (qualified ? issues : "unqualified_context");
            WriteBits(output, p);
            WriteBits(output, d);
            WriteBits(output, b);
            WriteBits(output, reconstructed);
            WriteBits(output, applied);
            output << '\n';
        }
        _mm_setcsr(original_mxcsr);

        std::ofstream summary(argv[3]);
        if (!summary) throw std::runtime_error("cannot open summary JSON");
        summary << "{\n  \"schema\": \"I-0006-writer-mirror-reconstruction-v1\",\n"
                << "  \"input\": \"" << JsonEscape(argv[1]) << "\",\n"
                << "  \"rows\": " << rows << ",\n"
                << "  \"qualified_rows\": " << qualified_rows << ",\n"
                << "  \"qualified_precommit_valid_rows\": " << valid_rows << ",\n"
                << "  \"all_qualified_rows_precommit_valid\": "
                << (qualified_rows > 0 && qualified_rows == valid_rows ? "true" : "false") << ",\n"
                << "  \"issue_counts\": {";
        bool first = true;
        for (const auto& [name, count] : issue_counts) {
            summary << (first ? "\n" : ",\n") << "    \"" << JsonEscape(name)
                    << "\": " << count;
            first = false;
        }
        if (!first) summary << '\n';
        summary << "  }\n}\n";
        return qualified_rows > 0 && qualified_rows == valid_rows ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << "ERROR: " << error.what() << '\n';
        return 2;
    }
}
