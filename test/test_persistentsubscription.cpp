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
#include <ls2-helpers.hpp>
#include "test_util.hpp"

#define TEST_SERVICE "com.webos.test_service"
#define TEST_CLIENT "com.webos.test_client"

using namespace pbnjson;

class TestService
{
public:
	TestService()
			: mTimeout {5, std::bind(&TestService::timer, this), mLoop.get()}
			, mService { LS::registerService(TEST_SERVICE) }
			, mSubscription { &mService }
			, mLunaClient { &mService }
	{
		mLunaClient.registerMethod("/","subscribe", this, &TestService::subscribe);
		mService.attachToLoop(mLoop.get());

		// Sleep some to allow service to register with the bus.
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	bool timer()
	{
		mSubscription.post(JObject{{"returnValue", true}, {"subscribed", true}});
		return true;
	}

	pbnjson::JValue subscribe(LSHelpers::JsonRequest& request)
	{
		bool subscribe;
		request.get("subscribe", subscribe);
		request.finishParseOrThrow(true);

		if (subscribe)
		{
			mSubscription.addSubscription(request.getMessage());
		}

		return JObject{{"subscribed", true}, {"firstResponse", true}};
	}

private:
	MainLoopT mLoop;
	Timeout mTimeout;
	LS::Handle mService;
	LSHelpers::SubscriptionPoint mSubscription;
	LSHelpers::ServicePoint mLunaClient;
};

TEST(TestPersistentSubscription, TestSubscribe)
{
	MainLoopT loop;

	auto handle = LS::registerService(TEST_CLIENT);
	handle.attachToLoop(loop.get());
	LSHelpers::PersistentSubscription subscription;

	volatile int count = 0;

	subscription.subscribe(&handle,
	                       "luna://" TEST_SERVICE "/subscribe",
	                       JObject{{"subscribe", true}},
	                       [&count](LSHelpers::JsonResponse& response)
	                       {
		                       count += 1;
	                       });
	std::this_thread::sleep_for(std::chrono::milliseconds(10));

	ASSERT_EQ(0, count); // Service not up, no responses
	ASSERT_FALSE(subscription.isServiceActive());

	//Start service and check that we get some responses.
	{
		TestService ts;
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		ASSERT_GT(count, 2);
		ASSERT_TRUE(subscription.isServiceActive());
	}

	//Stop service and check that no more responses.
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	int snapshot = count;
	ASSERT_FALSE(subscription.isServiceActive());

	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	ASSERT_EQ(snapshot, count);

	{
		//Resume service and check that get more responses
		TestService ts;
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		ASSERT_GT(count, snapshot);
		ASSERT_TRUE(subscription.isServiceActive());

		//Stop the subscription and check that no more responses.
		subscription.cancel();
		snapshot = count;
		ASSERT_FALSE(subscription.isServiceActive());

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		ASSERT_EQ(count, snapshot);
		ASSERT_FALSE(subscription.isServiceActive());
	}

	//Test repeat cancel behavior
	subscription.cancel();

	//Test does not crash if cancel unitialized
	{
		LSHelpers::PersistentSubscription sub;
		sub.cancel();
	}
}

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
