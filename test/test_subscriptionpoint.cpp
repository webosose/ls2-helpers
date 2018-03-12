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

#include "util.hpp"

#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>

#include <gtest/gtest.h>
#include <ls2-helpers/ls2-helpers.hpp>
#include "test_util.hpp"

#define TEST_CLASS_NAME "TestService"
std::atomic_uint g_counter{0};
std::atomic_uint g_sub_count{0};
pthread_barrier_t g_barrier;

class TestService
{
public:
	TestService() : _timeoutId{0}, _postId{1}, _mainloop{nullptr}, _deduplicate(false), _sameId(false)
	{
		_mainloop = g_main_loop_new(nullptr, FALSE);
		_service = LS::registerService("com.palm.test_subscription_service");

		LSMethod methods[] =
				{
						{ "stopCall", onStop, LUNA_METHOD_FLAGS_NONE },
						{ "subscribeCall", onRequest, LUNA_METHOD_FLAGS_NONE },
						{ },
				};
		_service.registerCategory("testCalls", methods, nullptr, nullptr);
		_service.setCategoryData("testCalls", this);
		_service.attachToLoop(_mainloop);

		g_sub_count = 0;
		EXPECT_FALSE(_sp.hasSubscribers());
	}

	~TestService()
	{
		g_main_loop_unref(_mainloop);
	}

	bool handleRequest(LSMessage *request)
	{
		if (LSMessageIsSubscription(request))
		{
			_sp.addSubscription(request);
			EXPECT_TRUE(_sp.hasSubscribers());
			LS::Message message{request};
			auto response = pbnjson::JObject{{"class", TEST_CLASS_NAME},
			                                 {"subscribed", true},
			                                 {"returnValue", true}};
			message.respond(response.stringify().c_str());
		}
		return true;
	}

	void postUpdate()
	{
		if (!_sameId)
		{
			_postId++;
		}
		_sp.post(pbnjson::JObject{{"id", _postId}}.stringify().c_str());
	}

	void run()
	{
		_timeoutId = g_timeout_add(100, onPostTimeout, this);
		g_main_loop_run(_mainloop);
	}

	void stop()
	{
		g_timeout_add(100, onStopTimeout, this);
	}

	static bool onStop(LSHandle *sh, LSMessage *request, void *context)
	{
		TestService * ts = static_cast<TestService *>(context);
		ts->stop();
		return true;
	}

	static bool onRequest(LSHandle *sh, LSMessage *request, void *context)
	{
		TestService * ts = static_cast<TestService *>(context);
		ts->handleRequest(request);
		return true;
	}

	static gboolean onPostTimeout(gpointer context)
	{
		TestService * ts = static_cast<TestService *>(context);
		ts->postUpdate();
		return G_SOURCE_CONTINUE;
	}

	static gboolean onStopTimeout(gpointer context)
	{
		TestService * ts = static_cast<TestService *>(context);
		//Timeout is on main loop, need to remove manually.
		g_source_remove(ts->_timeoutId);
		g_main_loop_quit(ts->_mainloop);
		return G_SOURCE_REMOVE;
	}

	void setSameId()
	{
		_sameId = true;
	}

	void setDeduplicate()
	{
		_sp.setDeduplicate(true);
	}

private:
	uint _timeoutId;
	int32_t _postId;
	GMainLoop * _mainloop;
	bool _deduplicate;
	bool _sameId;
	LS::Handle _service;
	LSHelpers::SubscriptionPoint _sp;
};

class TestSubscription
{
public:
	TestSubscription(const char* name = "com.palm.test_subscription_client")
	{
		_context = g_main_context_new();

		_client = LS::registerService(name);
		_client.attachToLoop(_context);

		_call = _client.callMultiReply(
				"luna://com.palm.test_subscription_service/testCalls/subscribeCall",
				R"({"subscribe":true})");
	}

	~TestSubscription()
	{
		g_main_context_unref(_context);
	}

	LS::Message get(unsigned long timeout)
	{
		return _call.get(timeout);
	}

	GMainContext* context()
	{
		return _context;
	}

	void verifyFirstResponse()
	{
		auto reply = _call.get();
		EXPECT_TRUE(bool(reply)) << "No response from test service";
		LSHelpers::JsonParser replyJSON{reply.getPayload()};
		EXPECT_TRUE(replyJSON.isValidJson());
		bool returnValue = false, isSubscribed = false;
		EXPECT_TRUE(replyJSON.get("returnValue", returnValue));
		EXPECT_TRUE(returnValue);
		EXPECT_TRUE(replyJSON.get("subscribed", isSubscribed));
		EXPECT_TRUE(isSubscribed);
		std::string serviceClass;
		EXPECT_TRUE(replyJSON.get("class", serviceClass));
		EXPECT_EQ(std::string(TEST_CLASS_NAME), serviceClass);
	}

	void close()
	{
		_call.cancel();
		LS::Call callStop = _client.callOneReply("luna://com.palm.test_subscription_service/testCalls/stopCall", "{}");
		callStop.get(200);
	}

	int expectPost(unsigned long timeout)
	{
		auto reply = _call.get(timeout);
		EXPECT_TRUE(bool(reply)) << "No post from test service";
		LSHelpers::JsonParser postJSON{reply.getPayload()};
		EXPECT_TRUE(postJSON.isValidJson());
		int32_t postId{0};
		EXPECT_TRUE(postJSON.get("id", postId));
		return postId;
	}

	void expectNoPost(unsigned long timeout)
	{
		ASSERT_FALSE(bool(_call.get(timeout))) << "Unexpected post form test service";
	}

private:
	GMainContext* _context;
	LS::Handle _client;
	LS::Call _call;
};

void serviceThreadFunc()
{
	try
	{
		TestService ts;
		ts.run();
	}
	catch (std::exception &e)
	{
		FAIL() << "TestService exception: " << e.what();
	}
	catch (...)
	{
		FAIL();
	}
}

void clientThreadFunc()
{
	auto context = mk_ptr(g_main_context_new(), g_main_context_unref);
	LS::Handle client = LS::registerService("");
	client.attachToLoop(context.get());

	LS::Call call = client.callMultiReply("luna://com.palm.test_subscription_service/testCalls/subscribeCall",
	                                            R"({"subscribe":true})");
	auto reply = call.get();
	EXPECT_TRUE(bool(reply)) << "No response from test service";
	LSHelpers::JsonParser replyJSON{reply.getPayload()};
	EXPECT_TRUE(replyJSON.isValidJson());
	bool returnValue = false, isSubscribed = false;
	EXPECT_TRUE(replyJSON.get("returnValue", returnValue));
	EXPECT_TRUE(returnValue);
	EXPECT_TRUE(replyJSON.get("subscribed", isSubscribed));
	EXPECT_TRUE(isSubscribed);
	std::string serviceClass;
	EXPECT_TRUE(replyJSON.get("class", serviceClass));
	EXPECT_EQ(std::string(TEST_CLASS_NAME), serviceClass);

	reply = call.get(200);
	EXPECT_TRUE(bool(reply)) << "No post from test service";
	LSHelpers::JsonParser postJSON{reply.getPayload()};
	EXPECT_TRUE(postJSON.isValidJson());
	int32_t postId{0};
	EXPECT_TRUE(postJSON.get("id", postId));
	EXPECT_LE(1, postId);
	++g_counter;

	pthread_barrier_wait(&g_barrier);

	call.cancel();
	--g_sub_count;
}

TEST(TestSubscriptionPoint, SubscriptionDisconnectTest)
{
	std::thread serviceThread{serviceThreadFunc};
	std::this_thread::sleep_for(std::chrono::milliseconds(10));

	TestSubscription client {""};
	client.verifyFirstResponse();
	client.expectPost(200);

	std::this_thread::sleep_for(std::chrono::milliseconds(10));

	LS::Handle client2 = LS::registerService("com.palm.test_subscription_client");
	client2.attachToLoop(client.context());

	LS::Call callStop = client2.callOneReply("luna://com.palm.test_subscription_service/testCalls/stopCall", "{}");
	callStop.get(200);

	serviceThread.join();
}

TEST(TestSubscriptionPoint, SubscriptionCancelTest)
{
	std::thread serviceThread{serviceThreadFunc};
	std::this_thread::sleep_for(std::chrono::milliseconds(10));

	TestSubscription client;
	client.verifyFirstResponse();
	client.expectPost(200);
	client.close();

	serviceThread.join();
}

TEST(TestSubscriptionPoint, SubscriptionTestMultiClientTest)
{
	pthread_barrier_init(&g_barrier, 0, 3);

	std::thread serviceThread{serviceThreadFunc};
	std::this_thread::sleep_for(std::chrono::milliseconds(10));

	std::thread client1{clientThreadFunc};
	std::this_thread::sleep_for(std::chrono::milliseconds(10));
	std::thread client2{clientThreadFunc};
	std::this_thread::sleep_for(std::chrono::milliseconds(10));
	std::thread client3{clientThreadFunc};

	client1.join();
	client2.join();
	client3.join();

	ASSERT_EQ(uint{3}, g_counter);
	GMainLoop * mainloop = g_main_loop_new(nullptr, FALSE);
	LS::Handle client = LS::registerService("com.palm.test_subscription_client");
	client.attachToLoop(mainloop);

	client.callOneReply("luna://com.palm.test_subscription_service/testCalls/stopCall", "{}");
	serviceThread.join();
	g_main_loop_unref(mainloop);

	pthread_barrier_destroy(&g_barrier);
}

TEST(TestSubscriptionPoint, PostBeforeSubscribe)
{
	MainLoopT main_loop;

	// Initialize the service with subscription point
	static LSMethod methods[] = {
			{ "method",
					[](LSHandle *sh, LSMessage *msg, void *ctx) -> bool
					{
						LSHelpers::SubscriptionPoint *s = static_cast<LSHelpers::SubscriptionPoint *>(ctx);
						LS::Message req(msg);
						req.respond(R"({"returnValue": true})");
						// We're going to post subscription response to previous clients and then
						// to add this one.
						// Expected that the new client doesn't get the response before it's been
						// subscribed.
						s->post(R"({"status": true})");
						s->addSubscription(req);
						return true;
					},
					LUNA_METHOD_FLAGS_NONE },
			{}
	};
	auto service = LS::registerService("com.webos.service");
	LSHelpers::SubscriptionPoint subscr;
	subscr.setDeduplicate(true);
	subscr.setServiceHandle(&service);
	service.registerCategory("/", methods, nullptr, nullptr);
	service.setCategoryData("/", &subscr);
	service.attachToLoop(main_loop.get());

	// Run the client
	auto client = LS::registerService("com.webos.client");
	client.attachToLoop(main_loop.get());
	auto call = client.callMultiReply("luna://com.webos.service/method", R"({"subscribe": true})");
	// Get normal response
	auto r = call.get(1000);
	ASSERT_NE(nullptr, r.get());
	EXPECT_STREQ(r.getPayload(), R"({"returnValue": true})");
	// See whether there's a subscription response (there shouldn't be any)
	r = call.get(1000);
	ASSERT_EQ(nullptr, r.get());

	main_loop.stop();
}

TEST(TestSubscriptionPoint, DestroyAfterPost)
{
	MainLoopT main_loop;

	// Initialize the service with subscription point
	static LSMethod methods[] = {
			{ "method",
					[](LSHandle *sh, LSMessage *msg, void *ctx) -> bool
					{
						LS::Message req(msg);
						req.respond(R"({"returnValue": true})");

						// Create a temporary subscription point
						LSHelpers::SubscriptionPoint subscr;
						subscr.setServiceHandle(static_cast<LS::Handle *>(ctx));

						subscr.addSubscription(req);
						subscr.post(R"({"status": true})");
						// Destroy the subscription point. The test will check that the last
						// response was delivered.
						return true;
					},
					LUNA_METHOD_FLAGS_NONE },
			{}
	};
	auto service = LS::registerService("com.webos.service");
	service.registerCategory("/", methods, nullptr, nullptr);
	service.setCategoryData("/", &service);
	service.attachToLoop(main_loop.get());

	// Run the client
	auto client = LS::registerService("com.webos.client");
	client.attachToLoop(main_loop.get());
	auto call = client.callMultiReply("luna://com.webos.service/method", R"({"subscribe": true})");
	// Get normal response
	auto r = call.get(1000);
	ASSERT_NE(nullptr, r.get());
	EXPECT_STREQ(r.getPayload(), R"({"returnValue": true})");
	// See whether there's a subscription response
	r = call.get(1000);
	ASSERT_NE(nullptr, r.get());
	EXPECT_STREQ(r.getPayload(), R"({"status": true})");
	// Nothing else is expected
	r = call.get(1000);
	ASSERT_EQ(nullptr, r.get());

	main_loop.stop();
}

TEST(TestSubscriptionPoint, PayloadDeduplicationDifferent)
{
	std::thread serviceThread{ [](){
		TestService ts;
		ts.setDeduplicate();
		ts.run();
	}};
	// Sleep some to allow service to register with the bus.
	std::this_thread::sleep_for(std::chrono::milliseconds(10));

	TestSubscription client;
	client.verifyFirstResponse();

	// Get 3 responses, they all should have increasing postId
	int prevId = -1;
	for (int i = 0; i < 3; i ++)
	{
		int postId = client.expectPost(200);
		ASSERT_LT(prevId, postId);
		prevId = postId;
	}

	client.close();
	serviceThread.join();
}

TEST(TestSubscriptionPoint, PayloadDeduplicationSame)
{
	std::thread serviceThread{ [](){
		TestService ts;
		ts.setDeduplicate();
		ts.setSameId();
		ts.run();
	}};
	// Sleep some to allow service to register with the bus.
	std::this_thread::sleep_for(std::chrono::milliseconds(10));

	TestSubscription client;
	client.verifyFirstResponse();

	client.expectPost(200);
	client.expectNoPost(1000);

	client.close();
	serviceThread.join();
}


TEST(TestSubscriptionPoint, PayloadDeduplicationSameNoDedup)
{
	std::thread serviceThread{ [](){
		TestService ts;
		ts.setSameId();
		ts.run();
	}};
	// Sleep some to allow service to register with the bus.
	std::this_thread::sleep_for(std::chrono::milliseconds(10));

	TestSubscription client;
	client.verifyFirstResponse();

	// Get 3 responses, they all should have the same postId
	int prevId = -1;
	for (int i = 0; i < 3; i ++)
	{
		int postId = client.expectPost(200);
		if (prevId == -1)
		{
			prevId = postId;
		}

		ASSERT_EQ(prevId, postId);
	}

	client.close();
	serviceThread.join();
}


int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
