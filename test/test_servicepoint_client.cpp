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
			: mTimeout {5, std::bind(&TestService::timer, this), mLoop.get()}
			, mService { LS::registerService(TEST_SERVICE) }
			, mSubscription { &mService }
			, mLunaClient { &mService }
	{
		mLunaClient.registerMethod("/","method", this, &TestService::method);
		mLunaClient.registerMethod("/","strictMethod", this, &TestService::strictMethod);
		mLunaClient.registerMethod("/","subscribe", this, &TestService::subscribe);
		mService.attachToLoop(mLoop.get());

		// Sleep some to allow service to register with the bus.
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	bool timer()
	{
		mSubscription.post(JObject{{"returnValue", true}, {"subscribed", true}}.stringify().c_str());
		return true;
	}

	pbnjson::JValue method(LSHelpers::JsonRequest& request)
	{
		std::string ping;
		request.get("ping", ping);
		request.finishParseOrThrow(false); //Allow additional parameters

		return JObject{{"pong", ping}, {"returnValue", true}};
	}

	pbnjson::JValue strictMethod(LSHelpers::JsonRequest& request)
	{
		std::string ping;
		request.get("ping", ping);
		request.finishParseOrThrow(true);//No additional parameters allowed

		return JObject{{"pong", ping}, {"returnValue", true}};
	}


	pbnjson::JValue subscribe(LSHelpers::JsonRequest& request)
	{
		bool subscribe;
		request.get("subscribe", subscribe);
		request.finishParseOrThrow(true);

		if (subscribe)
		{
			LS::Message message = request.getMessage();
			mSubscription.subscribe(message);
		}

		return JObject{{"subscribed", true}, {"firstResponse", true}};
	}

private:
	MainLoopT mLoop;
	Timeout mTimeout;
	LS::Handle mService;
	LS::SubscriptionPoint mSubscription;
	LSHelpers::ServicePoint mLunaClient;
};

TEST(TestSubscriptionPointClient, CallSuccess)
{
	TestService ts;
	MainLoopT loop;

	auto handle = LS::registerService(TEST_CLIENT);
	handle.attachToLoop(loop.get());
	LSHelpers::ServicePoint client { &handle };

	//Async call - set callback and wait 100 ms.
	bool callOk = false;
	client.callOneReply("luna://" TEST_SERVICE "/method",
	                    JObject {{"ping","1"},{"extra",true}},
	                    [&callOk](LSHelpers::JsonResponse& response)
	                    {
		                    callOk = response.isSuccess();
	                    });
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	ASSERT_TRUE(callOk);
}

TEST(TestSubscriptionPointClient, CallCancel)
{
	TestService ts;
	MainLoopT loop;

	auto handle = LS::registerService(TEST_CLIENT);
	handle.attachToLoop(loop.get());
	LSHelpers::ServicePoint client { &handle };

	//Async call - set callback and wait 100 ms.
	bool callOk = false;
	auto token = client.callOneReply("luna://" TEST_SERVICE "/method",
	                    JObject {{"ping","1"},{"extra",true}},
	                    [&callOk](LSHelpers::JsonResponse& response)
	                    {
		                    callOk = response.isSuccess();
	                    });
	client.cancelCall(token);
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	ASSERT_FALSE(callOk);
}

TEST(TestSubscriptionPointClient, CallMultiReply)
{
	TestService ts;
	MainLoopT loop;

	auto handle = LS::registerService(TEST_CLIENT);
	handle.attachToLoop(loop.get());
	LSHelpers::ServicePoint client { &handle };

	//Async call - set callback and wait 100 ms.
	volatile int responses = 0;
	auto token = client.callMultiReply("luna://" TEST_SERVICE "/subscribe",
	                    JObject {{"subscribe",true}},
	                    [&responses](LSHelpers::JsonResponse& response)
	                    {
		                    responses += 1;
	                    });
	std::this_thread::sleep_for(std::chrono::milliseconds(300));
	ASSERT_GT(responses, 5);

	//Test no more responses after cancel.
	client.cancelCall(token);
	int newResponses = responses;
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	ASSERT_EQ(newResponses, responses);
}

//Test call/response synchronization - the calls are done in one thread and responses processed in another thread.
TEST(TestSubscriptionPointClient, CallParallel)
{
	TestService ts;
	MainLoopT loop;

	auto handle = LS::registerService(TEST_CLIENT);
	handle.attachToLoop(loop.get());
	LSHelpers::ServicePoint client { &handle };

	volatile int responses = 0;
	for (int i = 0; i < 1000; i ++)
	{
		client.callOneReply("luna://" TEST_SERVICE "/method",
		                    JObject {{"ping","1"},{"extra",true}},
		                    [&responses](LSHelpers::JsonResponse& response)
		                    {
			                    responses += 1;
		                    });
	}
	//set callback and wait for some time.
	std::this_thread::sleep_for(std::chrono::milliseconds(300));
	ASSERT_EQ(1000, responses);
}

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
