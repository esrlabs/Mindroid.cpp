/*
 * Copyright (C) 2011 Daniel Himmelein
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

#include <mindroid/lang/Thread.h>
#include <mindroid/util/concurrent/Promise.h>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

namespace mindroid {

Thread::Thread(const sp<Runnable>& runnable, const sp<String>& name) :
        mName(name),
        mRunnable(runnable),
        mInterrupted(false) {
}

Thread::Thread(pthread_t thread) :
        mThread(thread),
        mInterrupted(false) {
}

void Thread::start() {
    if (mExecution == nullptr) {
        mSelf = this;
        mExecution = new Promise<bool>(sp<Executor>(nullptr));
        if (::pthread_create(&mThread, nullptr, &Thread::exec, this) == 0) {
#ifndef __APPLE__
            if (mName != nullptr) {
                ::pthread_setname_np(mThread, mName->c_str());
            }
#endif
        } else {
            mSelf.clear();
            object_cast<Promise<bool>>(mExecution)->complete(false);
        }
    }
}

void Thread::sleep(uint32_t milliseconds) {
    ::sleep(milliseconds / 1000);
    ::usleep((milliseconds % 1000) * 1000);
}

void Thread::join() const {
    if (mThread != 0) {
        try {
            mExecution->get();
        } catch (const ExecutionException& ignore) {
        }
        ::pthread_join(mThread, nullptr);
    }
}

void Thread::join(uint64_t millis) const {
    if (mThread != 0) {
        if (millis == 0) {
            join();
        } else {
            try {
                mExecution->get(millis);
            } catch (const TimeoutException& ignore) {
            } catch (const ExecutionException& ignore) {
            }
        }
    }
}

void* Thread::exec(void* args) {
    Thread* const self = (Thread*) args;
    sp<Runnable> runnable = (self->mRunnable != nullptr) ? self->mRunnable : self;
    sp<Promise<bool>> execution = object_cast<Promise<bool>>(self->mExecution);
    runnable->run();
    self->mSelf.clear();
    execution->complete(true);
    return nullptr;
}

void Thread::interrupt() {
    mInterrupted.store(true, std::memory_order_relaxed);
}

bool Thread::isInterrupted() const {
    return mInterrupted.load(std::memory_order_relaxed);
}

bool Thread::isAlive() const {
    return mSelf != nullptr;
}

uint64_t Thread::getId() const {
    return (uint64_t) ::pthread_self();
}

sp<Thread> Thread::currentThread() {
    return new Thread(::pthread_self());
}

void Thread::setName(const sp<String>& name) {
    if (name != nullptr) {
        mName = name;
#ifndef __APPLE__
        if (mThread != 0) {
            ::pthread_setname_np(mThread, mName->c_str());
        }
#endif
    }
}

void Thread::setPriority(int32_t priority) {
    sched_param schedulingParameters;
    std::memset(&schedulingParameters, 0, sizeof(schedulingParameters));
    int32_t policy = 0;
    ::pthread_getschedparam(mThread, &policy, &schedulingParameters);
    schedulingParameters.sched_priority = priority;
    ::pthread_setschedparam(mThread, policy, &schedulingParameters);
}

} /* namespace mindroid */
