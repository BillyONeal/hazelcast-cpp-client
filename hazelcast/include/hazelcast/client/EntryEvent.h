//
// Created by sancar koyunlu on 6/21/13.
// Copyright (c) 2013 hazelcast. All rights reserved.

#ifndef HAZELCAST_ENTRY_EVENT
#define HAZELCAST_ENTRY_EVENT

#include "hazelcast/client/impl/EventObject.h"
#include "hazelcast/client/connection/Member.h"
#include <boost/shared_ptr.hpp>

namespace hazelcast {
    namespace client {
        class HAZELCAST_API EntryEventType {
        public:
            enum Type {
                ADDED = 1, REMOVED = 2, UPDATED = 3, EVICTED = 4
            } value;

            EntryEventType() {

            }

            EntryEventType(Type value)
            :value(value) {

            }

            operator int() const {
                return value;
            }

            void operator = (int i) {
                switch (i) {
                    case 1:
                        value = ADDED;
                        break;
                    case 2:
                        value = REMOVED;
                        break;
                    case 3:
                        value = UPDATED;
                        break;
                    case 4:
                        value = EVICTED;
                        break;
                }
            }

        };

        template <typename K, typename V>
        class HAZELCAST_API EntryEvent : public impl::EventObject {
        public:

            EntryEvent(const std::string &name, const connection::Member &member, EntryEventType eventType, boost::shared_ptr<K> key, boost::shared_ptr<V> value)
            : impl::EventObject(name)
            , member(member)
            , eventType(eventType)
            , key(key)
            , value(value) {

            };

            EntryEvent(const std::string &name, const connection::Member &member, EntryEventType eventType, boost::shared_ptr<K> key, boost::shared_ptr<V> value, boost::shared_ptr<V> oldValue)
            : impl::EventObject(name)
            , member(member)
            , eventType(eventType)
            , key(key)
            , value(value)
            , oldValue(oldValue) {

            };

            /**
             * Returns the key of the entry event
             *
             * @return the key
             */
            const K &getKey() const {
                return *key;
            };

            /**
             * Returns the old value of the entry event
             *
             * @return
             */
            const V &getOldValue() const {
                return *oldValue;
            };

            /**
             * Returns the value of the entry event
             *
             * @return
             */
            const V &getValue() const {
                return *value;
            };

            /**
             * Returns the member fired this event.
             *
             * @return the member fired this event.
             */
            connection::Member getMember() const {
                return member;
            };

            /**
             * Return the event type
             *
             * @return event type
             */
            EntryEventType getEventType() const {
                return eventType;
            };

            /**
             * Returns the name of the map for this event.
             *
             * @return name of the map.
             */
            std::string getName() const {
                return getSource();
            };

        private:
            boost::shared_ptr<K> key;
            boost::shared_ptr<V> value;
            boost::shared_ptr<V> oldValue;
            EntryEventType eventType;
            connection::Member member;

        };
    }
}

#endif //__EntryEvent_H_
