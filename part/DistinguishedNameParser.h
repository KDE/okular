/*
    SPDX-FileCopyrightText: 2002 g10 Code GmbH
    SPDX-FileCopyrightText: 2004 Klarälvdalens Datakonsult AB
    SPDX-FileCopyrightText: 2021 g10 Code GmbH
    SPDX-FileCopyrightText: 2023 g10 Code GmbH
    SPDX-FileContributor: Author: Sune Stolborg Vuorela <sune@vuorela.dk>

    SPDX-License-Identifier: GPL-2.0-or-later

    Copied from poppler (pdf library)
*/

#ifndef DISTINGUISHEDNAMEPARSER_H
#define DISTINGUISHEDNAMEPARSER_H

#include <algorithm>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace DN
{
namespace detail
{

inline std::string_view removeLeadingSpaces(std::string_view view)
{
    auto pos = view.find_first_not_of(' ');
    if (pos > view.size()) {
        return {};
    }
    return view.substr(pos);
}

inline std::string_view removeTrailingSpaces(std::string_view view)
{
    auto pos = view.find_last_not_of(' ');
    if (pos > view.size()) {
        return {};
    }
    return view.substr(0, pos + 1);
}

inline unsigned char xtoi(unsigned char c)
{
    if (c <= '9') {
        return c - '0';
    }
    if (c <= 'F') {
        return c - 'A' + 10;
    }
    return c < 'a' + 10;
}

inline unsigned char xtoi(unsigned char first, unsigned char second)
{
    return 16 * xtoi(first) + xtoi(second);
}
// Parses a hex string into actual content
inline std::optional<std::string> parseHexString(std::string_view view)
{
    auto size = view.size();
    if (size == 0 || (size % 2 == 1)) {
        return std::nullopt;
    }
    // It is only supposed to be called with actual hex strings
    // but this is just to be extra sure
    auto endHex = view.find_first_not_of("1234567890abcdefABCDEF");
    if (endHex != std::string_view::npos) {
        return {};
    }
    std::string result;
    result.reserve(size / 2);
    for (size_t i = 0; i < (view.size() - 1); i += 2) {
        result.push_back(xtoi(view[i], view[i + 1]));
    }
    return result;
}

static const std::vector<std::pair<std::string_view, std::string_view>> &oidmap()
{
    static const std::vector<std::pair<std::string_view, std::string_view>> oidmap_ = {
        // clang-format off
    // keep them ordered by oid:
    {"NameDistinguisher", "0.2.262.1.10.7.20"   },
    {"EMAIL",             "1.2.840.113549.1.9.1"},
    {"CN",                "2.5.4.3"             },
    {"SN",                "2.5.4.4"             },
    {"SerialNumber",      "2.5.4.5"             },
    {"T",                 "2.5.4.12"            },
    {"D",                 "2.5.4.13"            },
    {"BC",                "2.5.4.15"            },
    {"ADDR",              "2.5.4.16"            },
    {"PC",                "2.5.4.17"            },
    {"GN",                "2.5.4.42"            },
    {"Pseudo",            "2.5.4.65"            },
        // clang-format on
    };
    return oidmap_;
}

static std::string_view attributeNameForOID(std::string_view oid)
{
    if (oid.substr(0, 4) == std::string_view {"OID."} || oid.substr(0, 4) == std::string_view {"oid."}) { // c++20 has starts_with. we don't have that yet.
        oid.remove_prefix(4);
    }
    for (const auto &m : oidmap()) {
        if (oid == m.second) {
            return m.first;
        }
    }
    return {};
}

/* Parse a DN and return an array-ized one.  This is not a validating
   parser and it does not support any old-stylish syntax; gpgme is
   expected to return only rfc2253 compatible strings. */
static std::pair<std::optional<std::string_view>, std::pair<std::string, std::string>> parse_dn_part(std::string_view stringv)
{
    std::pair<std::string, std::string> dnPair;
    auto separatorPos = stringv.find_first_of('=');
    if (separatorPos == 0 || separatorPos == std::string_view::npos) {
        return {}; /* empty key */
    }

    std::string_view key = stringv.substr(0, separatorPos);
    key = removeTrailingSpaces(key);
    // map OIDs to their names:
    if (auto name = attributeNameForOID(key); !name.empty()) {
        key = name;
    }

    dnPair.first = std::string {key};
    stringv = removeLeadingSpaces(stringv.substr(separatorPos + 1));
    if (stringv.empty()) {
        return {};
    }

    if (stringv.front() == '#') {
        /* hexstring */
        stringv.remove_prefix(1);
        auto endHex = stringv.find_first_not_of("1234567890abcdefABCDEF");
        if (!endHex || (endHex % 2 == 1)) {
            return {}; /* empty or odd number of digits */
        }
        auto value = parseHexString(stringv.substr(0, endHex));
        if (!value.has_value()) {
            return {};
        }
        stringv = stringv.substr(endHex);
        dnPair.second = value.value();
    } else if (stringv.front() == '"') {
        stringv.remove_prefix(1);
        std::string value;
        bool stop = false;
        while (!stringv.empty() && !stop) {
            switch (stringv.front()) {
            case '\\': {
                if (stringv.size() < 2) {
                    return {};
                }
                if (stringv[1] == '"') {
                    value.push_back('"');
                    stringv.remove_prefix(2);
                } else {
                    // it is a bit unclear in rfc2253 if escaped hex chars should
                    // be decoded inside quotes. Let's just forward the verbatim
                    // for now
                    value.push_back(stringv.front());
                    value.push_back(stringv[1]);
                    stringv.remove_prefix(2);
                }
                break;
            }
            case '"': {
                stop = true;
                stringv.remove_prefix(1);
                break;
            }
            default: {
                value.push_back(stringv.front());
                stringv.remove_prefix(1);
            }
            }
        }
        if (!stop) {
            // we have reached end of string, but never an actual ", so error out
            return {};
        }
        dnPair.second = value;
    } else {
        std::string value;
        bool stop = false;
        bool lastAddedEscapedSpace = false;
        while (!stringv.empty() && !stop) {
            switch (stringv.front()) {
            case '\\': //_escaping
            {
                stringv.remove_prefix(1);
                if (stringv.empty()) {
                    return {};
                }
                switch (stringv.front()) {
                case ',':
                case '=':
                case '+':
                case '<':
                case '>':
                case '#':
                case ';':
                case '\\':
                case '"':
                case ' ': {
                    if (stringv.front() == ' ') {
                        lastAddedEscapedSpace = true;
                    } else {
                        lastAddedEscapedSpace = false;
                    }
                    value.push_back(stringv.front());
                    stringv.remove_prefix(1);
                    break;
                }
                default: {
                    if (stringv.size() < 2) {
                        // this should be double hex-ish, but isn't.
                        return {};
                    }
                    if (std::isxdigit(stringv.front()) && std::isxdigit(stringv[1])) {
                        lastAddedEscapedSpace = false;
                        value.push_back(xtoi(stringv.front(), stringv[1]));
                        stringv.remove_prefix(2);
                        break;
                    } else {
                        // invalid escape
                        return {};
                    }
                }
                }
                break;
            }
            case '"':
                // unescaped " in the middle; not allowed
                return {};
            case ',':
            case '=':
            case '+':
            case '<':
            case '>':
            case '#':
            case ';': {
                stop = true;
                break; //
            }
            default:
                lastAddedEscapedSpace = false;
                value.push_back(stringv.front());
                stringv.remove_prefix(1);
            }
        }
        if (lastAddedEscapedSpace) {
            dnPair.second = value;
        } else {
            dnPair.second = std::string {removeTrailingSpaces(value)};
        }
    }
    return {stringv, dnPair};
}
}

using Result = std::vector<std::pair<std::string, std::string>>;

/* Parse a DN and return an array-ized one.  This is not a validating
   parser and it does not support any old-stylish syntax; gpgme is
   expected to return only rfc2253 compatible strings. */
static Result parseString(std::string_view string)
{
    Result result;
    while (!string.empty()) {
        string = detail::removeLeadingSpaces(string);
        if (string.empty()) {
            break;
        }

        auto [partResult, dnPair] = detail::parse_dn_part(string);
        if (!partResult.has_value()) {
            return {};
        }

        string = partResult.value();
        if (dnPair.first.size() && dnPair.second.size()) {
            result.emplace_back(std::move(dnPair));
        }

        string = detail::removeLeadingSpaces(string);
        if (string.empty()) {
            break;
        }
        switch (string.front()) {
        case ',':
        case ';':
        case '+':
            string.remove_prefix(1);
            break;
        default:
            // some unexpected characters here
            return {};
        }
    }
    return result;
}

/// returns the first value of a given key (note. there can be multiple)
/// or nullopt if key is not available
inline std::optional<std::string> FindFirstValue(const Result &dn, std::string_view key)
{
    auto first = std::find_if(dn.begin(), dn.end(), [&key](const auto &it) { return it.first == key; });
    if (first == dn.end()) {
        return {};
    }
    return first->second;
}
} // namespace DN
#endif // DISTINGUISHEDNAMEPARSER_H
