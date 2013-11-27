//
// Created by sancar koyunlu on 5/21/13.
// Copyright (c) 2013 sancar koyunlu. All rights reserved.


#ifndef HAZELCAST_CLUSTER_SERVICE
#define HAZELCAST_CLUSTER_SERVICE

#include "hazelcast/client/spi/ClientContext.h"
#include "hazelcast/client/spi/PartitionService.h"
#include "hazelcast/client/spi/ResponseStream.h"
#include "hazelcast/client/connection/Connection.h"
#include "hazelcast/client/connection/ClusterListenerThread.h"
#include "hazelcast/client/connection/ConnectionManager.h"
#include "hazelcast/client/exception/ServerException.h"
#include "hazelcast/client/Address.h"
#include "hazelcast/util/AtomicPointer.h"
#include "hazelcast/client/serialization/SerializationService.h"
#include "hazelcast/client/exception/InstanceNotActiveException.h"
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>
#include <set>

namespace hazelcast {
    namespace client {

        namespace spi {

            class ClusterService {
            public:
                ClusterService(spi::PartitionService &, spi::LifecycleService &, connection::ConnectionManager &, serialization::SerializationService &, ClientConfig &);

                void start();

                void stop();

                template< typename Response, typename Request >
                boost::shared_ptr<Response> sendAndReceive(const Request &object) {
                    while (active) {
                        connection::Connection *connection = NULL;
                        try {
                            connection = getRandomConnection();
                            serialization::Data request = serializationService.toData<Request>(&object);
                            connection->write(request);
                            serialization::Data responseData = connection->read();
                            boost::shared_ptr<Response > response = serializationService.toObject<Response>(responseData);
                            connectionManager.releaseConnection(connection);
                            return response;
                        } catch (exception::IOException &e) {
                            if (connection != NULL) {
                                std::cerr << "Error on conection : " << *connection << ", error: " << std::string(e.what()) << std::endl;
                            } else {
                                std::cerr << std::string(e.what()) << std::endl;
                            }
                            delete connection;
                            if (redoOperation || util::isRetryable(object)) {
                                std::cerr << "Retrying " << std::endl;
                                beforeRetry();
                                continue;
                            }
                        } catch (exception::IException &e) {
                            connectionManager.releaseConnection(connection);
                            throw e;
                        }
                    }
                    throw exception::InstanceNotActiveException("ClusterService:: sendAndReceive(const Request& object) ", "Instance is not active!!");
                };

                template< typename Response, typename Request >
                boost::shared_ptr<Response> sendAndReceive(const Address &address, const Request &object) {
                    while (active) {
                        connection::Connection *connection = NULL;
                        try {
                            connection = getConnection(address);
                            serialization::Data request = serializationService.toData<Request>(&object);
                            connection->write(request);
                            serialization::Data responseData = connection->read();
                            boost::shared_ptr<Response > response = serializationService.toObject<Response>(responseData);
                            connectionManager.releaseConnection(connection);
                            return response;
                        } catch (exception::IOException &e) {
                            if (connection != NULL) {
                                std::cerr << "Error on connection : " << *connection << ", error: " << std::string(e.what()) << std::endl;
                                delete connection;
                            }
                            if (redoOperation || util::isRetryable(object)) {
                                std::cerr << "Retrying ";
                                if (connection != NULL)
                                    std::cerr << ": last-connection" << *connection;
                                std::cerr << ", last-error: " << std::string(e.what()) << std::endl;
                                beforeRetry();
                                continue;
                            }
                        } catch (exception::IException &e) {
                            if (connection != NULL)
                                connectionManager.releaseConnection(connection);
                            throw e;
                        }
                    }
                    throw exception::InstanceNotActiveException("ClusterService::sendAndReceive(const Address& address, const Request& object)", "Instance is not active!!");

                };

                template< typename Request, typename ResponseHandler>
                void sendAndHandle(const Address &address, const Request &object, ResponseHandler &handler) {
                    std::auto_ptr<ResponseStream> stream(NULL);
                    while (stream.get() == NULL) {
                        if (!active) {
                            throw exception::InstanceNotActiveException("ClusterService:: sendAndHandle(const Address& address, const Request& object, ResponseHandler&  handler)", "Instance is not active!!");
                        }
                        connection::Connection *connection = NULL;
                        try {
                            connection = getConnection(address);
                            serialization::Data request = serializationService.toData<Request>(&object);
                            connection->write(request);
                            stream.reset(new ResponseStream(serializationService, connection));
                        } catch (exception::IOException &e) {
                            std::cerr << "Error on connection : " << *connection << ", error: " << std::string(e.what()) << std::endl;
                            delete connection;
                            if (redoOperation || util::isRetryable(object)) {
                                std::cerr << "Retrying : last-connection" << *connection << ", last-error: " << std::string(e.what()) << std::endl;
                                beforeRetry();
                                continue;
                            }
                        } catch (exception::IException &e) {
                            if (connection != NULL)
                                connectionManager.releaseConnection(connection);
                            throw e;
                        }

                    }

                    try {
                        handler.handle(*stream);
                    } catch (exception::IException &e) {
                        stream->end();
                        throw e;
                    }
                    stream->end();
                };

                template< typename Request, typename ResponseHandler>
                void sendAndHandle(const Request &object, ResponseHandler &handler) {
                    std::auto_ptr<ResponseStream> stream(NULL);
                    while (stream.get() == NULL) {
                        if (!active) {
                            throw exception::InstanceNotActiveException("ClusterService::sendAndHandle(const Request& object, ResponseHandler&  handler)", "Instance is not active!!");
                        }
                        connection::Connection *connection = NULL;
                        try {
                            connection = getRandomConnection();
                            serialization::Data request = serializationService.toData<Request>(&object);
                            connection->write(request);
                            stream.reset(new ResponseStream(serializationService, connection));
                        } catch (exception::IOException &e) {
                            std::cerr << "Error on connection : " << *connection << ", error: " << std::string(e.what()) << std::endl;
                            if (connection != NULL)
                                delete connection;
                            if (redoOperation || util::isRetryable(object)) {
                                std::cerr << "Retrying : last-connetcion" << *connection << ", last-error: " << std::string(e.what()) << std::endl;
                                beforeRetry();
                                continue;
                            }
                        } catch (exception::IException &e) {
                            if (connection != NULL)
                                connectionManager.releaseConnection(connection);
                            throw e;
                        }
                    }
                    try {
                        handler.handle(*stream);
                    } catch (exception::IException &e) {
                        stream->end();
                        throw e;
                    }
                    stream->end();
                };

                template< typename Response, typename Request>
                boost::shared_ptr<Response> sendAndReceiveFixedConnection(connection::Connection *connection, const Request &request) {
                    serialization::Data data = serializationService.toData<Request>(&request);
                    connection->write(data);
                    serialization::Data response = connection->read();
                    return serializationService.toObject<Response>(response);
                }

                std::auto_ptr<Address> getMasterAddress();

                void addMembershipListener(MembershipListener *listener);

                bool removeMembershipListener(MembershipListener *listener);

                bool isMemberExists(const Address &address);

                connection::Member getMember(const std::string &uuid);

                std::vector<connection::Member> getMemberList();

                friend class connection::ClusterListenerThread;

                static const int RETRY_COUNT = 20;
                static const int RETRY_WAIT_TIME = 500;
            private:
                void setMembers(const std::map<Address, connection::Member, addressComparator > &map);

                connection::ConnectionManager &connectionManager;
                serialization::SerializationService &serializationService;
                ClientConfig &clientConfig;
                spi::LifecycleService &lifecycleService;
                spi::PartitionService &partitionService;

                connection::ClusterListenerThread clusterThread;

                protocol::Credentials &credentials;
                std::map<Address, connection::Member, addressComparator > members;
                std::set< MembershipListener *> listeners;
                boost::mutex listenerLock;
                boost::mutex membersLock;
                const bool redoOperation;
                boost::atomic<bool> active;

                void fireMembershipEvent(connection::MembershipEvent &membershipEvent);

                connection::Connection *getConnection(const Address &address);

                connection::Connection *getRandomConnection();

                connection::Connection *connectToOne(const std::vector<Address> &socketAddresses);

                void beforeRetry();

            };

        }
    }
}

#endif //HAZELCAST_CLUSTER_SERVICE
