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
			: mService { new LS::Handle(LS::registerService(TEST_SERVICE)) }
			, mLunaClient { new LSHelpers::ServicePoint( mService.get() ) }
	{
		mLunaClient->registerMethod("/","method", this, &TestService::method);
		mLunaClient->registerMethod("/","strictMethod", this, &TestService::strictMethod);
		mLunaClient->registerMethod("/","deferred", this, &TestService::deferred);
		mLunaClient->registerMethod("/","shutdown", this, &TestService::shutdown);
		mService->attachToLoop(mLoop.get());

			// Sleep some to allow service to register with the bus.
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	~TestService()
	{
		if (mDeferred)
		{
			mDeferred(JObject{{"returnValue", true}});
			mDeferred = nullptr;
		}

		mLoop.stop();
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

	// Shut down the service and send response
	pbnjson::JValue shutdown(LSHelpers::JsonRequest& request)
	{
		mLunaClient.release();
		mLoop.stop();
		mService.release();
		return true;
	}

	pbnjson::JValue deferred(LSHelpers::JsonRequest& request)
	{
		std::string ping;
		request.get("ping", ping);
		request.finishParseOrThrow(false); //Allow additional parameters

		//Send response to previous call ;)
		if (mDeferred)
		{
			mDeferred(JObject{{"returnValue", true}});
			mDeferred = nullptr;
		}

		//Do not reply, hold on to the request.
		mDeferred = request.defer();
		return true;
	}

private:
	MainLoopT mLoop;
	std::unique_ptr<LS::Handle> mService;
	std::unique_ptr<LSHelpers::ServicePoint> mLunaClient;
	LSHelpers::JsonRequest::DeferredResponseFunction mDeferred;
};


TEST(TestSubscriptionPointService, CallSuccess)
{
	TestService ts;
	MainLoopT loop;

	auto client = LS::registerService(TEST_CLIENT);
	client.attachToLoop(loop.get());

	//Nonstrict
	{
		auto call = client.callMultiReply("luna://" TEST_SERVICE "/method",
		                                  R"({"ping":"1", "extra":true})");
		auto reply = call.get();
		ASSERT_TRUE(reply.getPayload());
		LSHelpers::JsonParser p{reply.getPayload()};
		std::string pong;
		bool rv;
		ASSERT_TRUE(bool(p.get("pong", pong)));
		ASSERT_TRUE(bool(p.get("returnValue", rv)));
		ASSERT_TRUE(bool(p.finishParse(true)));
		ASSERT_EQ("1", pong);
		ASSERT_EQ(true, rv);

		//No second message
		reply = call.get(200);
		ASSERT_FALSE(bool(reply));
	}

	//Strict
	{
		auto call = client.callMultiReply("luna://" TEST_SERVICE "/strictMethod",
		                                  R"({"ping":"1"})");
		auto reply = call.get();
		ASSERT_TRUE(reply.getPayload());
		LSHelpers::JsonParser p{reply.getPayload()};
		std::string pong;
		bool rv;
		ASSERT_TRUE(bool(p.get("pong", pong)));
		ASSERT_TRUE(bool(p.get("returnValue", rv)));
		ASSERT_TRUE(bool(p.finishParse(true)));
		ASSERT_EQ("1", pong);
		ASSERT_EQ(true, rv);

		//No second message
		reply = call.get(200);
		ASSERT_FALSE(bool(reply));
	}
}

TEST(TestSubscriptionPointService, CallFail)
{
	TestService ts;
	MainLoopT loop;

	auto client = LS::registerService(TEST_CLIENT);
	client.attachToLoop(loop.get());

	//Non strict - non-json payload
	{
		auto call = client.callMultiReply("luna://" TEST_SERVICE "/method", R"(this is not json)");
		auto reply = call.get();
		ASSERT_TRUE(reply.getPayload());
		LSHelpers::JsonParser p{reply.getPayload()};
		bool rv;
		std::string em;
		int ec;
		ASSERT_TRUE(bool(p.get("returnValue", rv)));
		ASSERT_TRUE(bool(p.get("errorMessage", em)));
		ASSERT_TRUE(bool(p.get("errorCode", ec)));
		ASSERT_TRUE(bool(p.finishParse(true)));
		ASSERT_FALSE(rv);

		//No second message
		reply = call.get(200);
		ASSERT_FALSE(bool(reply));
	}

	//Strict - extra fields
	{
		auto call = client.callMultiReply("luna://" TEST_SERVICE "/strictMethod", R"({"ping":"1", "extra":true})");
		auto reply = call.get();
		ASSERT_TRUE(reply.getPayload());
		LSHelpers::JsonParser p{reply.getPayload()};
		bool rv;
		std::string em;
		int ec;
		ASSERT_TRUE(bool(p.get("returnValue", rv)));
		ASSERT_TRUE(bool(p.get("errorMessage", em)));
		ASSERT_TRUE(bool(p.get("errorCode", ec)));
		ASSERT_TRUE(bool(p.finishParse(true)));
		ASSERT_FALSE(rv);

		//No second message
		reply = call.get(200);
		ASSERT_FALSE(bool(reply));
	}
}

TEST(TestSubscriptionPointService, CallDeferred)
{
	TestService ts;
	MainLoopT loop;

	auto client = LS::registerService(TEST_CLIENT);
	client.attachToLoop(loop.get());

	auto call = client.callMultiReply("luna://" TEST_SERVICE "/deferred", R"({"ping":"1", "extra":true})");
	//No response
	auto reply = call.get(200);
	ASSERT_FALSE(bool(reply));

	//Run second request
	auto call1 = client.callMultiReply("luna://" TEST_SERVICE "/deferred", R"({"ping":"1", "extra":true})");
	//No response
	reply = call1.get(200);
	ASSERT_FALSE(bool(reply));

	//First call should have a response now.
	reply = call.get(200);
	ASSERT_TRUE(bool(reply));
}

TEST(TestSubscriptionPointService, CallShutdown)
{
	TestService ts;
	MainLoopT loop;

	auto client = LS::registerService(TEST_CLIENT);
	client.attachToLoop(loop.get());

	//Should get a response even if service is shut down during call handler.
	auto call = client.callMultiReply("luna://" TEST_SERVICE "/shutdown", R"({})");
	auto reply = call.get(200);
	ASSERT_TRUE(bool(reply));
}

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
