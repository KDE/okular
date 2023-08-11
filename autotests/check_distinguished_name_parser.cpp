/*
    SPDX-FileCopyrightText: 2023 g10 Code GmbH
    SPDX-FileContributor: Sune Stolborg Vuorela <sune@vuorela.dk>

    SPDX-License-Identifier: GPL-2.0-or-later

    Copied from poppler (pdf library)
*/
#include "../part/DistinguishedNameParser.h"

#include <QtTest/QtTest>
#include <iostream>

class TestDistinguishedNameParser : public QObject
{
    Q_OBJECT
public:
    explicit TestDistinguishedNameParser(QObject *parent = nullptr)
        : QObject(parent)
    {
    }
private Q_SLOTS:
    // The big set of input/output. Several of the helper functions can be tested independently
    void testParser();
    void testParser_data();

    void testRemoveLeadingSpaces();
    void testRemoveLeadingSpaces_data();

    void testRemoveTrailingSpaces();
    void testRemoveTrailingSpaces_data();

    void testParseHexString();
    void testParseHexString_data();
};

Q_DECLARE_METATYPE(DN::Result);
Q_DECLARE_METATYPE(std::string);
Q_DECLARE_METATYPE(std::optional<std::string>);

void TestDistinguishedNameParser::testParser()
{
    QFETCH(std::string, inputData);
    QFETCH(DN::Result, expectedResult);

    auto result = DN::parseString(inputData);
    QCOMPARE(result, expectedResult);
}

void TestDistinguishedNameParser::testParser_data()
{
    QTest::addColumn<std::string>("inputData");
    QTest::addColumn<DN::Result>("expectedResult");

    QTest::newRow("empty") << std::string {} << DN::Result {};
    QTest::newRow("CN=Simple") << std::string {"CN=Simple"} << DN::Result {{"CN", "Simple"}};
    QTest::newRow("CN=Name with spaces") << std::string {"CN=Name with spaces"} << DN::Result {{"CN", "Name with spaces"}};
    QTest::newRow("CN=Simple,O=Silly") << std::string {"CN=Simple,O=Silly"} << DN::Result {{"CN", "Simple"}, {"O", "Silly"}};
    QTest::newRow("CN=Steve Kille,O=Isode Limited,C=GB") << std::string {"CN=Steve Kille,O=Isode Limited,C=GB"} << DN::Result {{"CN", "Steve Kille"}, {"O", "Isode Limited"}, {"C", "GB"}};
    QTest::newRow("CN=some.user@example.com, O=MyCompany, L=San Diego,ST=California, C=US")
        << std::string {"CN=some.user@example.com, O=MyCompany, L=San Diego,ST=California, C=US"} << DN::Result {{"CN", "some.user@example.com"}, {"O", "MyCompany"}, {"L", "San Diego"}, {"ST", "California"}, {"C", "US"}};
    QTest::newRow("Multi valued") << std::string {"OU=Sales+CN=J. Smith,O=Widget Inc.,C=US"}
                                  << DN::Result {{"OU", "Sales"}, {"CN", "J. Smith"}, {"O", "Widget Inc."}, {"C", "US"}}; // This is technically wrong, but probably good enough for now
    QTest::newRow("Escaping comma") << std::string {"CN=L. Eagle,O=Sue\\, Grabbit and Runn,C=GB"} << DN::Result {{"CN", "L. Eagle"}, {"O", "Sue, Grabbit and Runn"}, {"C", "GB"}};
    QTest::newRow("Escaped trailing space") << std::string {"CN=Trailing space\\ "} << DN::Result {{"CN", "Trailing space "}};
    QTest::newRow("Escaped quote") << std::string {"CN=Quotation \\\" Mark"} << DN::Result {{"CN", "Quotation \" Mark"}};

    QTest::newRow("CN=Simple with escaping") << std::string {"CN=S\\69mpl\\65\\7A"} << DN::Result {{"CN", "Simplez"}};
    QTest::newRow("SN=Lu\\C4\\8Di\\C4\\87") << std::string {"SN=Lu\\C4\\8Di\\C4\\87"} << DN::Result {{"SN", "Lučić"}};
    QTest::newRow("CN=\"Quoted name\"") << std::string {"CN=\"Quoted name\""} << DN::Result {{"CN", "Quoted name"}};
    QTest::newRow("CN=\" Leading and trailing spacees \"") << std::string {"CN=\" Leading and trailing spaces \""} << DN::Result {{"CN", " Leading and trailing spaces "}};
    QTest::newRow("Comma in quotes") << std::string {"CN=\"Comma, inside\""} << DN::Result {{"CN", "Comma, inside"}};
    QTest::newRow("forbidden chars in quotes") << std::string {"CN=\"Forbidden !@#$%&*()<>[]{},.?/\\| chars\""} << DN::Result {{"CN", "Forbidden !@#$%&*()<>[]{},.?/\\| chars"}};
    QTest::newRow("Quoted quotation") << std::string {"CN=\"Quotation \\\" Mark\""} << DN::Result {{"CN", "Quotation \" Mark"}};
    QTest::newRow("Quoted quotation") << std::string {"CN=\"Quotation \\\" Mark\\\" Multiples\""} << DN::Result {{"CN", "Quotation \" Mark\" Multiples"}};

    QTest::newRow("frompdf1") << std::string {"2.5.4.97=#5553742D49644E722E20444520313233343735323233,CN=TeleSec PKS eIDAS QES CA 5,O=Deutsche Telekom AG,C=DE"}
                              << DN::Result {{"2.5.4.97", "USt-IdNr. DE 123475223"}, {"CN", "TeleSec PKS eIDAS QES CA 5"}, {"O", "Deutsche Telekom AG"}, {"C", "DE"}};
    QTest::newRow("frompdf2") << std::string {"2.5.4.5=#34,CN=Koch\\, Werner,2.5.4.42=#5765726E6572,2.5.4.4=#4B6F6368,C=DE"} << DN::Result {{"SerialNumber", "4"}, {"CN", "Koch, Werner"}, {"GN", "Werner"}, {"SN", "Koch"}, {"C", "DE"}};
    QTest::newRow("frompdf2a") << std::string {"2.5.4.5=#34,CN=Koch\\, Werner,oid.2.5.4.42=#5765726E6572,OID.2.5.4.4=#4B6F6368,C=DE"}
                               << DN::Result {{"SerialNumber", "4"}, {"CN", "Koch, Werner"}, {"GN", "Werner"}, {"SN", "Koch"}, {"C", "DE"}};

    // weird spacing
    QTest::newRow("CN =Simple") << std::string {"CN =Simple"} << DN::Result {{"CN", "Simple"}};
    QTest::newRow("CN= Simple") << std::string {"CN= Simple"} << DN::Result {{"CN", "Simple"}};
    QTest::newRow("CN=Simple ") << std::string {"CN=Simple "} << DN::Result {{"CN", "Simple"}};
    QTest::newRow("CN=Simple,") << std::string {"CN=Simple,"} << DN::Result {{"CN", "Simple"}};
    QTest::newRow("CN=Simple, O=Silly") << std::string {"CN=Simple, O=Silly"} << DN::Result {{"CN", "Simple"}, {"O", "Silly"}};

    // various malformed
    QTest::newRow("CN=Simple\\") << std::string {"CN=Simple\\"} << DN::Result {};
    QTest::newRow("CN=") << std::string {"CN="} << DN::Result {};
    QTest::newRow("CN=Simple\\X") << std::string {"CN=Simple\\X"} << DN::Result {};
    QTest::newRow("CN=Simple, O") << std::string {"CN=Simple, O"} << DN::Result {};
    QTest::newRow("CN=Sim\"ple") << std::string {"CN=Sim\"ple, O"} << DN::Result {};
    QTest::newRow("CN=Simple\\a") << std::string {"CN=Simple\\a"} << DN::Result {};
    QTest::newRow("=Simple") << std::string {"=Simple"} << DN::Result {};
    QTest::newRow("CN=\"Simple") << std::string {"CN=\"Simple"} << DN::Result {};
    QTest::newRow("CN=\"Simple") << std::string {"CN=\"Simple\\"} << DN::Result {};
    QTest::newRow("unquoted quotation in quotation") << std::string {"CN=\"Quotation \" Mark\""} << DN::Result {};
}

void TestDistinguishedNameParser::testRemoveLeadingSpaces()
{
    QFETCH(std::string, input);
    QFETCH(std::string, expectedOutput);

    auto result = DN::detail::removeLeadingSpaces(input);
    QCOMPARE(result, expectedOutput);
}
void TestDistinguishedNameParser::testRemoveLeadingSpaces_data()
{
    QTest::addColumn<std::string>("input");
    QTest::addColumn<std::string>("expectedOutput");

    QTest::newRow("Empty") << std::string {} << std::string {};
    QTest::newRow("No leading spaces") << std::string {"horse"} << std::string {"horse"};
    QTest::newRow("Some spaces") << std::string {"    horse"} << std::string {"horse"};
    QTest::newRow("Some leading and trailing") << std::string {"    horse   "} << std::string {"horse   "};
}

void TestDistinguishedNameParser::testRemoveTrailingSpaces()
{
    QFETCH(std::string, input);
    QFETCH(std::string, expectedOutput);

    auto result = DN::detail::removeTrailingSpaces(input);
    QCOMPARE(result, expectedOutput);
}
void TestDistinguishedNameParser::testRemoveTrailingSpaces_data()
{
    QTest::addColumn<std::string>("input");
    QTest::addColumn<std::string>("expectedOutput");

    QTest::newRow("Empty") << std::string {} << std::string {};
    QTest::newRow("No leading spaces") << std::string {"horse"} << std::string {"horse"};
    QTest::newRow("Some spaces") << std::string {"horse    "} << std::string {"horse"};
    QTest::newRow("Some leading and trailing") << std::string {"    horse   "} << std::string {"    horse"};
}

void TestDistinguishedNameParser::testParseHexString()
{
    QFETCH(std::string, input);
    QFETCH(std::optional<std::string>, expectedOutput);

    auto result = DN::detail::parseHexString(input);
    QCOMPARE(result, expectedOutput);
}

void TestDistinguishedNameParser::testParseHexString_data()
{
    QTest::addColumn<std::string>("input");
    QTest::addColumn<std::optional<std::string>>("expectedOutput");

    QTest::newRow("4") << std::string {"34"} << std::optional<std::string>("4");
    QTest::newRow("Koch") << std::string {"4B6F6368"} << std::optional<std::string>("Koch");
    QTest::newRow("USt-IdNr. DE 123475223") << std::string {"5553742D49644E722E20444520313233343735323233"} << std::optional<std::string>("USt-IdNr. DE 123475223");

    // various baddies
    QTest::newRow("empty") << std::string {} << std::optional<std::string> {};
    QTest::newRow("FFF") << std::string {"FFF"} << std::optional<std::string> {};
    QTest::newRow("F") << std::string {"F"} << std::optional<std::string> {};
    QTest::newRow("XX") << std::string {"XX"} << std::optional<std::string> {};
}

QTEST_GUILESS_MAIN(TestDistinguishedNameParser);
#include "check_distinguished_name_parser.moc"
