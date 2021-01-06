/*
 * blocking_queue.h
 *
 *  Created on: Dec 5, 2014
 *      Author: andreas
 */

#ifndef BLOCKING_QUEUE_H_
#define BLOCKING_QUEUE_H_

#include <iostream>
#include <queue>

#include <Poco/Mutex.h>
#include <Poco/Condition.h>
#include <Poco/ScopedLock.h>

template<typename T>
struct is_pointer { static const bool value = false; };

template<typename T>
struct is_pointer<T*> { static const bool value = true; };

template<typename T>
class BlockingQueue
{
    protected:
        std::queue<T> queue_;
        Poco::Mutex mutex_;
        Poco::Condition queue_empty_;

    public:
        BlockingQueue()
        {

        }

        virtual ~BlockingQueue()
        {

        }

        void clear()
        {
            Poco::ScopedLock<Poco::Mutex> lock(mutex_);
            while(queue_.size() > 0)
            {
                if(is_pointer<T>::value)
                {
                    delete queue_.front();
                }
                queue_.pop();
            }
        }

        void put(T item)
        {
            Poco::ScopedLock<Poco::Mutex> lock(mutex_);
            queue_.push(item);
            queue_empty_.signal();
        }

        T take(uint32_t timeout = 0)
        {
            Poco::ScopedLock<Poco::Mutex> lock(mutex_);
            if(queue_.empty())
            {
                if(timeout == 0)
                {
                    queue_empty_.wait(mutex_);
                }
                else
                {
                    queue_empty_.wait(mutex_, timeout);
                }
            }

            T item = queue_.front();
            queue_.pop();
            return item;
        }
};

#endif /* BLOCKING_QUEUE_H_ */
