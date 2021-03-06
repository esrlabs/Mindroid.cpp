/*
 * Copyright (C) 2019 E.S.R.Labs
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>
#include <mindroid/os/ServiceManager.h>
#include <mindroid/runtime/system/Runtime.h>
#include <mindroid/util/logging/LoggerService.h>

namespace mindroid {
class TestLoggerService : public LoggerService {
public:
    TestLoggerService() {
        sInstance = this;
    }
    ~TestLoggerService() override {
        if (sInstance != this) {
            abort();
        }
        sInstance = nullptr;
    }
    static TestLoggerService* sInstance;
};

TestLoggerService* TestLoggerService::sInstance = nullptr;
} // namespace mindroid

using namespace mindroid;

CLASS(mindroid, TestLoggerService);

void startLoggerService() {
    sp<IServiceManager> serviceManager = ServiceManager::getServiceManager();

    sp<ArrayList<sp<String>>> logFlags = new ArrayList<sp<String>>();
    logFlags->add(String::valueOf("timestamp"));
    sp<Intent> logger = new Intent(Logger::ACTION_LOG);
    logger->setComponent(new ComponentName("mindroid", "LoggerService"))
            ->putExtra("name", Context::LOGGER_SERVICE->toString())
            ->putExtra("process", "main")
            ->putExtra("logBuffer", Log::LOG_ID_MAIN)
            ->putExtra("logPriority", Log::DEBUG)
            ->putStringArrayListExtra("logFlags", logFlags)
            ->putExtra("consoleLogging", true);
    serviceManager->startSystemService(logger);
    ServiceManager::waitForSystemService(Context::LOGGER_SERVICE);
}

void shutdownLoggerService() {
    sp<IServiceManager> serviceManager = ServiceManager::getServiceManager();

    sp<Intent> logger = new Intent();
    logger->setComponent(new ComponentName("mindroid", "LoggerService"));
    serviceManager->stopSystemService(logger);
    ServiceManager::waitForSystemServiceShutdown(Context::LOGGER_SERVICE);
}

void startTestLoggerService() {
    sp<IServiceManager> serviceManager = ServiceManager::getServiceManager();

    sp<ArrayList<sp<String>>> logFlags = new ArrayList<sp<String>>();
    logFlags->add(String::valueOf("timestamp"));
    sp<Intent> logger = new Intent(Logger::ACTION_LOG);
    logger->setComponent(new ComponentName("mindroid", "TestLoggerService"));
    serviceManager->startSystemService(logger)->get();
}

void shutdownTestLoggerService() {
    sp<IServiceManager> serviceManager = ServiceManager::getServiceManager();

    sp<Intent> logger = new Intent();
    logger->setComponent(new ComponentName("mindroid", "TestLoggerService"));
    serviceManager->stopSystemService(logger)->get();
}

TEST(Logger, checkLogger) {
    Runtime::start(1, nullptr);

    sp<ServiceManager> serviceManager = new ServiceManager();
    serviceManager->start();

    startLoggerService();
    Log::d("Test", "This is a test");
    {
        sp<Logger> logger = new Logger();
        logger->assumeThat("Test", "This is a test", 10000);
    }
    shutdownLoggerService();

    ASSERT_EQ(serviceManager->getSystemService(Context::LOGGER_SERVICE), nullptr);

    serviceManager->shutdown();
    Runtime::shutdown();
}

/*
 * Regression test that verifies TestLoggerService does not leak memory.
 */
TEST(Logger, loggerServiceNoMemoryLeak) {
    ASSERT_TRUE(TestLoggerService::sInstance == nullptr);
    {
        Runtime::start(1, nullptr);
        sp<ServiceManager> serviceManager = new ServiceManager();
        serviceManager->start();

        startTestLoggerService();
        ASSERT_TRUE(TestLoggerService::sInstance != nullptr);
        shutdownTestLoggerService();

        serviceManager->shutdown();
        Runtime::shutdown();
    }
    ASSERT_TRUE(TestLoggerService::sInstance == nullptr);
}
