// Copyright (c) 2016-2018 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>
#include <ls2-helpers/ls2-helpers.hpp>

using namespace std;
using namespace pbnjson;

/**
 * Basic positive test. Check all basic types can be parsed.
 */
TEST(TestJsonParser, JsonParserBasicTest)
{
	std::string payload = R"json({
"objectValue":{},
"boolValue":false,
"intValue":1234,
"stringValue":"Test string",
"doubleValue":42.5,
"arrayValue":["string", 789, true, null]
})json";

	LSHelpers::JsonParser jp(payload);
	EXPECT_FALSE(jp.hasError());

	bool boolTest = true;
	jp.get("boolValue", boolTest);
	EXPECT_FALSE(jp.hasError());
	EXPECT_FALSE(boolTest);

	int32_t intTest{0};
	jp.get("intValue", intTest);
	EXPECT_FALSE(jp.hasError());
	EXPECT_EQ(1234, intTest);

	double doubleTest{0.0};
	jp.get("doubleValue", doubleTest);
	EXPECT_FALSE(jp.hasError());
	EXPECT_DOUBLE_EQ(42.5, doubleTest);

	std::string stringTest;
	jp.get("stringValue", stringTest);
	EXPECT_FALSE(jp.hasError());
	EXPECT_EQ(std::string("Test string"), stringTest);

	JValue objectTest;
	jp.get("objectValue", objectTest);
	EXPECT_FALSE(jp.hasError());
	EXPECT_TRUE(objectTest.isObject());
	EXPECT_FALSE(objectTest.isNull());
	EXPECT_EQ(ssize_t{0}, objectTest.objectSize());

	JValue arrayTest = JObject();
	jp.get("arrayValue", arrayTest);
	EXPECT_FALSE(jp.hasError());
	EXPECT_TRUE(arrayTest.isArray());
	EXPECT_EQ(ssize_t{4}, arrayTest.arraySize());

	EXPECT_TRUE(jp.finishParse(true));
	EXPECT_FALSE(jp.hasError());
}

TEST(TestJsonParser, JsonParserOptionalTest)
{
	std::string payload = R"json({
"nullValue":null,
"objectValue":{},
"boolValue":false,
"intValue":1234,
"stringValue":"Test string",
"doubleValue":42.5,
"arrayValue":["string", 789, true, null]
})json";

	bool isSet;

	LSHelpers::JsonParser jp(payload);
	EXPECT_FALSE(jp.hasError());

	JValue nullTest = JObject();
	isSet = true;
	jp.get("nullValue", nullTest).optional(true).allowNull(true).checkValueRead(isSet);
	EXPECT_FALSE(jp.hasError());
	EXPECT_FALSE(isSet);

	JValue objectTest = JValue();
	isSet = false;
	jp.get("objectValue", objectTest).optional(true).allowNull(true).checkValueRead(isSet);
	EXPECT_FALSE(jp.hasError());
	EXPECT_TRUE(isSet);
	EXPECT_FALSE(objectTest.isNull());
	EXPECT_EQ(ssize_t{0}, objectTest.objectSize());

	int32_t intTest{0};
	isSet = false;
	jp.get("intValue", intTest).optional(true).checkValueRead(isSet);
	EXPECT_FALSE(jp.hasError());
	EXPECT_TRUE(isSet);
	EXPECT_EQ(1234, intTest);

	intTest = 0;
	isSet = false;
	jp.get("intValueMissing", intTest).optional(true).checkValueRead(isSet);
	EXPECT_FALSE(jp.hasError());
	EXPECT_FALSE(isSet);
	EXPECT_EQ(0, intTest);

	EXPECT_TRUE(jp.finishParse(false));
	EXPECT_FALSE(jp.hasError());
}

TEST(TestJsonParser, JsonParserArraylTest)
{
	std::string payload = R"json({
"intArray":[1,2,3],
"stringArray": ["a","b","c"],
"doubleArray": [0.1,0.2,0.3],
"jvalueArray":["string", 789, true]
})json";

	LSHelpers::JsonParser jp(payload);
	EXPECT_FALSE(jp.hasError());
	bool isSet = false;

	std::vector<int> intTest;
	jp.getArray("intArray", intTest);
	EXPECT_FALSE(jp.hasError());
	EXPECT_EQ(3U, intTest.size());
	EXPECT_EQ(1, intTest[0]);
	EXPECT_EQ(2, intTest[1]);
	EXPECT_EQ(3, intTest[2]);

	std::vector<double> doubleTest;
	jp.getArray("doubleArray", doubleTest);
	EXPECT_FALSE(jp.hasError());
	EXPECT_EQ(3U, doubleTest.size());
	EXPECT_EQ(0.1, doubleTest[0]);
	EXPECT_EQ(0.2, doubleTest[1]);
	EXPECT_EQ(0.3, doubleTest[2]);

	std::vector<std::string> stringTest;
	jp.getArray("stringArray", stringTest);
	EXPECT_FALSE(jp.hasError());
	EXPECT_EQ(3U, stringTest.size());
	EXPECT_STREQ("a", stringTest[0].c_str());
	EXPECT_STREQ("b", stringTest[1].c_str());
	EXPECT_STREQ("c", stringTest[2].c_str());

	std::vector<JValue> jvalueTest;
	jp.getArray("jvalueArray", jvalueTest);
	EXPECT_STREQ("", jp.getError().c_str());
	EXPECT_EQ(3U, jvalueTest.size());
	EXPECT_EQ(JValue("string"), jvalueTest[0]);
	EXPECT_EQ(JValue(789), jvalueTest[1]);
	EXPECT_EQ(JValue(true), jvalueTest[2]);

	std::vector<int> missingTest;
	jp.getArray("noAraray", missingTest).optional().defaultValue({42, 24}).checkValueRead(isSet);
	EXPECT_FALSE(jp.hasError());
	EXPECT_FALSE(isSet);
	EXPECT_EQ(2U, missingTest.size());
	EXPECT_EQ(42, missingTest[0]);
	EXPECT_EQ(24, missingTest[1]);

	//Optional and no default value.
	missingTest.clear();
	jp.getArray("noAraray", missingTest).optional().checkValueRead(isSet);
	EXPECT_FALSE(jp.hasError());
	EXPECT_FALSE(isSet);
	EXPECT_EQ(0U, missingTest.size());

	//Test optional but value exists
	jp.getArray("intArray", intTest).optional().defaultValue({42, 24}).checkValueRead(isSet);
	EXPECT_FALSE(jp.hasError());
	EXPECT_TRUE(isSet);
	EXPECT_EQ(3U, intTest.size());
	EXPECT_EQ(1, intTest[0]);
	EXPECT_EQ(2, intTest[1]);
	EXPECT_EQ(3, intTest[2]);

	EXPECT_TRUE(jp.finishParse(false));
	EXPECT_FALSE(jp.hasError());
}

TEST(TestJsonParser, JsonParserInvalidJsonTest)
{
	std::string payloadInvalid = R"json({"}})json";
	std::string payloadMissing = R"json({"Bar": 1})json";

	int foo;
	int bar;
	bool fooSet;
	bool barSet;

	LSHelpers::JsonParser jp(payloadInvalid);
	EXPECT_FALSE(jp.isValidJson());

	EXPECT_NO_THROW(jp.get("Fooo", foo).checkValueRead(fooSet));
	EXPECT_FALSE(fooSet);

	EXPECT_FALSE(jp.finishParse(false));
	EXPECT_FALSE(jp.finishParse(true));

	EXPECT_THROW(jp.finishParseOrThrow(false), LSHelpers::JsonParseError);
	EXPECT_THROW(jp.finishParseOrThrow(true), LSHelpers::JsonParseError);

	LSHelpers::JsonParser jp2(payloadMissing);
	EXPECT_TRUE(jp2.isValidJson());

	EXPECT_NO_THROW(jp2.get("Bar", bar).checkValueRead(barSet));
	EXPECT_EQ(1, bar);
	EXPECT_TRUE(barSet);

	EXPECT_NO_THROW(jp2.get("Fooo", foo).checkValueRead(fooSet));
	EXPECT_FALSE(fooSet);

	EXPECT_FALSE(jp2.finishParse(false));
	EXPECT_FALSE(jp2.finishParse(true));
	EXPECT_THROW(jp2.finishParseOrThrow(false), LSHelpers::JsonParseError);
	EXPECT_THROW(jp2.finishParseOrThrow(true), LSHelpers::JsonParseError);
}


TEST(TestJsonParser, JsonParserIntegerLimitsTest)
{
	std::string payload = R"json({
"0": 0,
"-1": -1,
"-128": -128,
"-129": -129,
"127": 127,
"128": 128,
"255": 255,
"256": 256,
"-32768": -32768,
"-32769": -32769,
"32767": 32767,
"32768": 32768,
"65535": 65535,
"65536": 65536,
"2.5": 2.5,
"foo": "foo"
})json";

	uint8_t uint8;
	int8_t int8;
	uint16_t uint16;
	int16_t int16;
	int32_t int32;
	bool valueRead;

	LSHelpers::JsonParser jp(payload);
	EXPECT_TRUE(jp.isValidJson());

	int8 = 10;
	jp.get("2.5", int8).checkValueRead(valueRead);
	EXPECT_EQ(10, int8);
	EXPECT_FALSE(valueRead);

	int8 = 10;
	jp.get("foo", int8).checkValueRead(valueRead);
	EXPECT_EQ(10, int8);
	EXPECT_FALSE(valueRead);

	uint8 = 10;
	jp.get("0", uint8);
	EXPECT_EQ(0, uint8);

	uint8 = 10;
	jp.get("-1", uint8).checkValueRead(valueRead);
	EXPECT_EQ(10, uint8);
	EXPECT_FALSE(valueRead);

	uint8 = 10;
	jp.get("255", uint8).checkValueRead(valueRead);
	EXPECT_EQ(255, uint8);
	EXPECT_TRUE(valueRead);

	uint8 = 10;
	jp.get("256", uint8).checkValueRead(valueRead);
	EXPECT_EQ(10, uint8);
	EXPECT_FALSE(valueRead);

	int8 = 10;
	jp.get("-128", int8);
	EXPECT_EQ(-128, int8);

	int8 = 10;
	jp.get("-129", int8).checkValueRead(valueRead);
	EXPECT_EQ(10, int8);
	EXPECT_FALSE(valueRead);

	int8 = 10;
	jp.get("127", int8).checkValueRead(valueRead);
	EXPECT_EQ(127, int8);
	EXPECT_TRUE(valueRead);

	int8 = 10;
	jp.get("128", int8).checkValueRead(valueRead);
	EXPECT_EQ(10, int8);
	EXPECT_FALSE(valueRead);


	uint16 = 10;
	jp.get("0", uint16);
	EXPECT_EQ(0, uint16);

	uint16 = 10;
	jp.get("-1", uint16).checkValueRead(valueRead);
	EXPECT_EQ(10, uint16);
	EXPECT_FALSE(valueRead);

	uint16 = 10;
	jp.get("65535", uint16).checkValueRead(valueRead);
	EXPECT_EQ(65535, uint16);
	EXPECT_TRUE(valueRead);

	uint16 = 10;
	jp.get("65536", uint16).checkValueRead(valueRead);
	EXPECT_EQ(10, uint16);
	EXPECT_FALSE(valueRead);

	int16 = 10;
	jp.get("-32768", int16);
	EXPECT_EQ(-32768, int16);

	int16 = 10;
	jp.get("-32769", int16).checkValueRead(valueRead);
	EXPECT_EQ(10, int16);
	EXPECT_FALSE(valueRead);

	int16 = 10;
	jp.get("32767", int16).checkValueRead(valueRead);
	EXPECT_EQ(32767, int16);
	EXPECT_TRUE(valueRead);

	int16 = 10;
	jp.get("32768", int16).checkValueRead(valueRead);
	EXPECT_EQ(10, int16);
	EXPECT_FALSE(valueRead);

	int32 = 10;
	jp.get("32768", int32).checkValueRead(valueRead);
	EXPECT_EQ(32768, int32);
	EXPECT_TRUE(valueRead);
}


TEST(TestJsonParser, JsonParserMinMaxTest)
{
	std::string payload = R"json({
"0": 0,
"100": 100,
"2.5": 2.5,
"foo": "foo"
})json";


	int8_t int8;
	double dd;
	std::string str;

	LSHelpers::JsonParser jp(payload);
	EXPECT_TRUE(jp.isValidJson());

	int8 = 10;
	jp.clearError();
	EXPECT_TRUE((bool)jp.get("0", int8).min(0).max(100));
	EXPECT_EQ(0, int8);

	int8 = 10;
	jp.clearError();
	EXPECT_TRUE((bool)jp.get("100", int8).min(0).max(100));
	EXPECT_EQ(100, int8);

	int8 = 10;
	jp.clearError();
	EXPECT_FALSE((bool)jp.get("100", int8).min(0).max(99));

	int8 = 10;
	jp.clearError();
	EXPECT_TRUE((bool)jp.get("100", int8).min(100).max(100));
	EXPECT_EQ(100, int8);

	int8 = 10;
	jp.clearError();
	EXPECT_FALSE((bool)jp.get("100", int8).min(101).max(200));

	dd = 10;
	jp.clearError();
	EXPECT_TRUE((bool)jp.get("2.5", dd).min(0).max(100));
	EXPECT_EQ(2.5, dd);

	dd = 10;
	jp.clearError();
	EXPECT_TRUE((bool)jp.get("2.5", dd).min(0).max(2.5));
	EXPECT_EQ(2.5, dd);

	dd = 10;
	jp.clearError();
	EXPECT_FALSE((bool)jp.get("2.5", dd).min(0).max(2.4999));

	dd = 10;
	jp.clearError();
	EXPECT_TRUE((bool)jp.get("2.5", dd).min(2.5).max(2.5));
	EXPECT_EQ(2.5, dd);

	dd = 10;
	jp.clearError();
	EXPECT_FALSE((bool)jp.get("2.5", dd).min(2.5001).max(3));
}

TEST(TestJsonParser, JsonParserDefaultValueTest)
{
	std::string payload = R"json({})json";

	bool b;
	int8_t int8;
	double dd;
	std::string str;
	bool valueRead;

	LSHelpers::JsonParser jp(payload);
	EXPECT_TRUE(jp.isValidJson());

	int8 = 10;
	jp.get("0", int8).optional(true).defaultValue(1).checkValueRead(valueRead);
	EXPECT_FALSE(valueRead);
	EXPECT_EQ(1, int8);

	b = false;
	jp.get("0", b).optional(true).defaultValue(true).checkValueRead(valueRead);
	EXPECT_FALSE(valueRead);
	EXPECT_EQ(true, b);

	dd = 10;
	jp.get("0", dd).optional(true).defaultValue(1).checkValueRead(valueRead);
	EXPECT_FALSE(valueRead);
	EXPECT_EQ(1, dd);

	str = "10";
	jp.get("0", str).optional(true).defaultValue("1").checkValueRead(valueRead);
	EXPECT_FALSE(valueRead);
	EXPECT_EQ("1", str);
}

TEST(TestJsonParser, JsonParserValueListTest)
{
	std::string payload = R"json({
"objectValue":{},
"boolValue":false,
"intValue":1234,
"stringValue":"Test string",
"doubleValue":42.5,
"arrayValue":["string", 789, true, null]
})json";

	int i;
	std::string s;
	bool valueRead;

	LSHelpers::JsonParser jp(payload);
	EXPECT_TRUE(jp.isValidJson());

	i = 10;
	jp.clearError();
	EXPECT_TRUE((bool) jp.get("intValue", i).allowedValues({0, 1, 2, 1234}).checkValueRead(
			valueRead));
	EXPECT_TRUE(valueRead);
	EXPECT_EQ(1234, i);

	i = 10;
	jp.clearError();
	EXPECT_FALSE((bool)jp.get("intValue", i).allowedValues({0,1,2}));

	s = "10";
	jp.clearError();
	EXPECT_TRUE((bool) jp.get("stringValue", s).allowedValues({"a", "b",
	                                                             "Test string"}).checkValueRead(
			valueRead));
	EXPECT_TRUE(valueRead);
	EXPECT_EQ("Test string", s);

	s = "10";
	jp.clearError();
	EXPECT_FALSE((bool)jp.get("stringValue", s).allowedValues({"a","b","Test string1"}).optional(true).defaultValue("100"));

	s = "10";
	jp.clearError();
	EXPECT_TRUE((bool) jp.get("stringValue1", s).allowedValues({"a", "b", "Test string1"}).optional(true).defaultValue("100").checkValueRead(valueRead));
	EXPECT_FALSE(valueRead);
	EXPECT_EQ("100", s);
}

TEST(TestJsonParser, JsonParserValueMapTest)
{
	std::string payload = R"json({
"intValue":1234,
"intValue2":12345,
"stringValue":"many",
"stringValue1":"zero",
"stringValue2":"one"
})json";

	int i;
	std::string s;

	LSHelpers::JsonParser jp(payload);
	EXPECT_TRUE(jp.isValidJson());

	std::array<std::pair<int, string>, 2> INT_ARRAY {{{0, "zero"}, {1234, "many"}}};

	s = "";
	jp.clearError();
	EXPECT_TRUE((bool)jp.getAndMap("intValue", s, INT_ARRAY));
	EXPECT_EQ("many", s);

	s = "";
	jp.clearError();
	bool success = jp.getAndMap< int, std::string >("intValue", s, {{0, "zero"}, {1234, "many"}});
	EXPECT_TRUE(success);
	EXPECT_EQ("many", s);

	s = "";
	jp.clearError();
	EXPECT_FALSE((bool)jp.getAndMap("intValue2", s, INT_ARRAY));
	EXPECT_EQ("", s);

	std::unordered_map<string, int> STRING_MAP {{"zero", 0}, {"many", 1234}};

	i = 10;
	jp.clearError();
	EXPECT_TRUE((bool)jp.getAndMap("stringValue", i, STRING_MAP));
	EXPECT_EQ(1234, i);

	i = 10;
	jp.clearError();
	EXPECT_TRUE((bool)jp.getAndMap("stringValue1", i, STRING_MAP));
	EXPECT_EQ(0, i);

	i = 10;
	jp.clearError();
	EXPECT_FALSE((bool)jp.getAndMap("stringValue2", i, STRING_MAP));

}

TEST(TestJsonParser, JsonParserGetFromString)
{
	std::string payload = R"json({
"objectValue":"{}",
"boolValue":"false",
"intValue":"1234",
"doubleValue":"42.5"
})json";

	LSHelpers::JsonParser jp(payload);
	EXPECT_FALSE(jp.hasError());

	bool boolTest = true;
	jp.getFromString("boolValue", boolTest);
	EXPECT_FALSE(jp.hasError());
	EXPECT_FALSE(boolTest);

	int32_t intTest{0};
	jp.getFromString("intValue", intTest);
	EXPECT_FALSE(jp.hasError());
	EXPECT_EQ(1234, intTest);

	intTest = 0;
	jp.get("intValue", intTest);
	EXPECT_FALSE(jp.hasError());
	EXPECT_EQ(1234, intTest);

	double doubleTest{0.0};
	jp.getFromString("doubleValue", doubleTest);
	EXPECT_FALSE(jp.hasError());
	EXPECT_DOUBLE_EQ(42.5, doubleTest);

	doubleTest = 0.0;
	jp.get("doubleValue", doubleTest);
	EXPECT_FALSE(jp.hasError());
	EXPECT_DOUBLE_EQ(42.5, doubleTest);

	std::string stringTest;
	jp.getFromString("stringValue", stringTest);
	EXPECT_TRUE(jp.hasError());
	EXPECT_EQ(std::string(""), stringTest);
	jp.clearError();

	JValue objectTest;
	jp.getFromString("objectValue", objectTest);
	EXPECT_FALSE(jp.hasError());
	EXPECT_TRUE(objectTest.isObject());
	EXPECT_FALSE(objectTest.isNull());
	EXPECT_EQ(ssize_t{0}, objectTest.objectSize());

	EXPECT_TRUE(jp.finishParse(false));
	EXPECT_FALSE(jp.hasError());
}


int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
