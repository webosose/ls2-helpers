// Copyright (c) 2008-2018 LG Electronics, Inc.
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
#include "test_util.hpp"

#define TEST_SERVICE "com.webos.test_service"
#define TEST_CLIENT "com.webos.test_client"

using namespace pbnjson;

class TestService
{
public:
	TestService()
			: mService { LS::registerService(TEST_SERVICE) }
			, mLunaClient { &mService }
			, mTimeout {5, std::bind(&TestService::timer, this), mLoop.get()}
	{
		mLunaClient.registerSignal("/test","activated");
		mService.attachToLoop(mLoop.get());

		// Sleep some to allow service to register with the bus.
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}


	bool timer()
	{
		mLunaClient.sendSignal("/test","activated", JObject{{"signal", true}});
		return true;
	}

private:
	MainLoopT mLoop;
	LS::Handle mService;
	LSHelpers::ServicePoint mLunaClient;
	Timeout mTimeout;
};

TEST(TestSubscriptionPointSignal, CallSuccess)
{
	MainLoopT loop;

	auto handle = LS::registerService(TEST_CLIENT);
	handle.attachToLoop(loop.get());
	LSHelpers::ServicePoint client { &handle };

	volatile int responseCount = 0;
	auto token = client.subscribeToSignal("/test", "activated",
	                    [&responseCount](LSHelpers::JsonResponse& response)
	                    {
		                    bool isSingal = false;
		                    response.get("signal", isSingal);
		                    response.finishParseOrThrow();

		                    responseCount += 1;
	                    });

	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	ASSERT_EQ(0, responseCount);

	//Start the service for 100 ms
	{
		TestService ts;
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	//Allow some time for data on pipes to settle.
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	ASSERT_GT(responseCount, 2);

	//Check that no more responses after service is down.
	int responses = responseCount;
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	ASSERT_EQ(responseCount, responses);

	//Check more signal responses when service goes back up
	{
		TestService ts;
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	ASSERT_GT(responseCount, responses);
	responses = responseCount;

	//Check no more responses after cancelling the call
	client.cancelCall(token);

	{
		TestService ts;
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	ASSERT_EQ(responseCount, responses);
}

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
