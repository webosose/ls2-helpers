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
#include <pbnjson.hpp>
#include <ls2-helpers/ls2-helpers.hpp>
#include "test_util.hpp"

#define  TEST_SERVICE  "com.webos.test_service"
#define  TEST_CLIENT   "com.webos.test_client"

using namespace pbnjson;

class TestService
{
public:
	TestService()
	{
		mHandle = nullptr;
		LSError error;
		LSErrorInit(&error);
		if (!LSRegister(TEST_SERVICE, &mHandle, &error))
		{
			LSErrorPrint(&error, stdout);
		}

		LSMethod methods[] = {
				{"method", &TestService::methodHandler, LUNA_METHOD_FLAGS_NONE},
				{}
		};

		if (!LSRegisterCategory(mHandle, "/", methods, nullptr, nullptr, &error))
		{
			LSErrorPrint(&error, stdout);
		}
		if (!LSGmainAttach(mHandle, mLoop.get(), &error))
		{
			LSErrorPrint(&error, stdout);
		}

		// Sleep some to allow service to register with the bus.
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}

	~TestService()
	{
		LSUnregister(mHandle, nullptr);
	}

	static bool methodHandler(LSHandle *sh, LSMessage *msg, void *category_context)
	{
		LSMessageRespond(msg, "{\"returnValue\":true}", nullptr);
		return true;
	}

private:
	MainLoopT mLoop;
	LSHandle* mHandle;
};

volatile bool callOk = false;

bool callResponse(LSHandle *sh, LSMessage *msg, void *context)
{
	callOk = true;
	return true;
}

TEST(TestCApi, Test)
{
	TestService ts;
	MainLoopT loop;

	LSError error;
	LSMessageToken token;
	LSHandle* handle = nullptr;

	LSErrorInit(&error);
	ASSERT_TRUE(LSRegister(TEST_CLIENT, &handle, &error));
	ASSERT_TRUE(LSGmainAttach(handle, loop.get(), &error));

	//Async call - set callback and wait 100 ms.
	ASSERT_TRUE(LSCall(handle, "luna://" TEST_SERVICE "/method", "{}", callResponse, nullptr, &token, &error));
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	ASSERT_TRUE(callOk);

	ASSERT_TRUE(LSUnregister(handle, &error));
}

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
