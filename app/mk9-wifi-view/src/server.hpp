#include <iostream>
#include <chrono>
#include <thread>
#include <cstring>
#include <map>
#include <functional>

#include "zmq.hpp"
#include "zmq_addon.hpp"

#include "flatbuffers/flexbuffers.h"


#include "efp.hpp"


#include "plotlib.hpp"




#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


/*
메인 스레드
	통신 객체 생성
	객체에 리스트 보낼것을 요청

통신 스레드
	독립적으로 계속 돌아감
	리스트 요청이 있다면 보냄
	기본은 수신
	디바이스의 데이터 송신 시 콜백으로 메인에 공급

*/





/*
(deviceID did)

publish:
	did/ca - myID
	did/cslist - signal ID want to receive

subscribe:
	did - signal
	did/da - device connection ON/OFF
	did/slist - signal IDs to receive

veiwer ON
	start network thread
		init lib, connect and subscribe
		publish: did/clid   my ID

		wait for did/stat {}
		wait for did/slist
		publish: did/cslist
		var ready = did/stat did/slist did/cslist done

		while (true)
			while (!ready)
				// handle no client status
				wait for did/stat {}
				wait for did/slist
				publish: did/cslist

			get()
			get did	

	
	when did/stat
		publish: did/clid

now on / on
start ->
<-live
ok->

on / now on
<- start 
live ->
<- ok

now on / now on
start -> <- start 
live -> <-live
ok-> <- ok
*/




enum SignalType : int16_t
{
	t_yet = 0, // net specified yet
	t_bool,
	t_float,
	t_double,
	t_int,
	t_int64
};

typedef int8_t SignalID;
typedef int8_t SignalThreadID;



class ThreadSignal
{
public:
	std::string thread_name;
	std::vector<std::string> signal_name;
	std::vector<SignalType> signal_type;
};








class ClientViewer
{
public:	
	
	using SignalRequestList = flexbuffers::Builder;

	using CB_C = std::function<void(const std::string)>;
	using CB_DC = std::function<void(const std::string)>;
	using CB_NT = std::function<void(const std::string, const flexbuffers::Vector &)>;
	using CB_S = std::function<void(const std::string, const flexbuffers::Vector &)>;

	ClientViewer(
		
		std::string host_address, 
		const CB_C &connection_callback,
		const CB_DC &disconnection_callback,
		const CB_NT &nametype_callback, 
		const CB_S &signal_callback) :
		ctx{}, 
		ci{},
		connection_thread{start_background_connection, std::ref(ctx), std::ref(ci), host_address,
			CallbackList {
				connection_callback,
				disconnection_callback,
				nametype_callback,
				signal_callback
			}}
	{
	}

	~ClientViewer()
	{
		ci.set_close();	

		// todo: shutdown here is safe? should warp thread body with try{}?
		// Cease any blocking operations in progress.
		ctx.shutdown();

		// Do a shutdown, if needed and destroy the context.
		ctx.close();
		connection_thread.join();
	}
	
	void publish_slist(const std::string &device_id, const std::vector<uint8_t> &sql)
	{
		ci.pub_list(device_id, sql);
	}



private:

	struct CallbackList
	{
		const CB_C connection_callback;
		const CB_DC disconnection_callback;
		const CB_NT nametype_callback;
		const CB_S signal_callback;
	};

	enum DeviceMessageType : int8_t 
	{
		dmt_connection,
		dmt_alive,
		dmt_disconnection,

		dmt_specification,
		dmt_signaldata,
		dmt_misc,

		dmt_unknown
	};
	enum ServerMessageType : int8_t 
	{
		smt_connection,
		smt_alive,
		smt_disconnection,

		smt_specification,

		smt_misc,

		smt_unknown
	};
	

	class ConnectionInfo
	{
	public:
		void set_close() 
		{
			std::lock_guard<std::mutex> locked(m_info);
			close_server = true;
		}
		bool get_close() 
		{
			std::lock_guard<std::mutex> locked(m_info);
			return close_server;
		}
		
		void pub_list(const std::string &device_id, const std::vector<uint8_t> &sql)
		{
			std::lock_guard<std::mutex> locked(m_info);
			auto iter = dc.find(device_id);
			if (iter != dc.end())
			{
				iter->second.request = true;
				iter->second.sql = sql;
			}
			// id not found as connection removed id but yet notified.
		}

		template<typename F1>
		void sub_list(F1 &send)
		{
			std::lock_guard<std::mutex> locked(m_info);
			for (auto &iter : dc)
			{
				DeviceConnection &d = iter.second;
				if (d.request)
				{	
					send(iter.first, ServerMessageType::smt_specification, d.sql);
					d.request = false;
				}
			}
		}

		bool sub_connection(const std::string &id)
		{
			std::lock_guard<std::mutex> locked(m_info);
			auto iter = dc.find(id);
			if (iter != dc.end())
			{
				return false;
			}

			auto res = dc.emplace(id, DeviceConnection());
			return res.second;
		}

		bool open_connection(const std::string &id)
		{
			std::lock_guard<std::mutex> locked(m_info);
			auto iter = dc.find(id);
			if (iter != dc.end())
			{
				dc.erase(iter);
			}

			auto res = dc.emplace(id, DeviceConnection());
			return res.second;
		}

		void close_connection(const std::string &id)
		{
			std::lock_guard<std::mutex> locked(m_info);
			auto iter = dc.find(id);
			if (iter != dc.end())
			{
				dc.erase(iter);
			}
		}

		void update_alive(const std::string &id)
		{
			std::lock_guard<std::mutex> locked(m_info);
			dc[id].time = 0;
		}

	private:

		struct DeviceConnection
		{
			//zmq::socket_t thread_socket;
			//std::thread connection_thread;
			int32_t time{0};

			// mutex
			bool request{false};
			std::vector<uint8_t> sql;
			
		};
		std::mutex m_info;

		std::map<std::string, DeviceConnection> dc;

		bool close_server{false};
		
	};

	class connect_monitor_t : public zmq::monitor_t {
	public:
		connect_monitor_t(zmq::socket_t &socket, std::string addr) :
		socket(socket), addr(addr)
		{
			init(socket, "inproc://monitor_pull", ZMQ_EVENT_ALL);

		}

		void on_monitor_started() override
		{
			// socket.bind(addr);
			std::cout << "on_monitor_started" << std::endl;
		}   

		void on_event_connected(const zmq_event_t& event,
								const char* addr) override
		{
			std::cout << "on_event_connected " << addr << std::endl;
		}
		void on_event_connect_delayed(const zmq_event_t& event,
								const char* addr) override
		{
			std::cout << "on_event_connect_delayed " << addr << std::endl;
		}		
		void on_event_connect_retried(const zmq_event_t& event,
								const char* addr) override
		{
			std::cout << "on_event_connect_retried " << addr << std::endl;
		}
		void on_event_listening(const zmq_event_t& event,
								const char* addr) override
		{
			connection = true;
			std::cout << "on_event_listening " << addr << std::endl;
		}
		void on_event_bind_failed(const zmq_event_t& event,
								const char* addr) override
		{
			std::cout << "on_event_bind_failed " << addr << std::endl;
		}
		void on_event_accepted(const zmq_event_t& event,
								const char* addr) override
		{
			std::cout << "on_event_accepted " << addr << std::endl;
		}
		void on_event_accept_failed(const zmq_event_t& event,
								const char* addr) override
		{
			std::cout << "on_event_accept_failed " << addr << std::endl;
		}
		void on_event_closed(const zmq_event_t& event,
								const char* addr) override
		{
			connection = false;
			std::cout << "on_event_closed " << addr << std::endl;
		}
		void on_event_close_failed(const zmq_event_t& event,
								const char* addr) override
		{
			std::cout << "on_event_close_failed " << addr << std::endl;
		}
		void on_event_disconnected(const zmq_event_t& event,
								const char* addr) override
		{
			std::cout << "on_event_disconnected " << addr << std::endl;
		}

		bool check_events()
		{
			for(int i = 0; i < 10; ++i) 
			{
				if(!check_event())
					break;
			}

			return connection;
		}
		zmq::socket_t &socket;
		std::string addr;

		bool connection{false};
	};


	static void start_background_connection(
		zmq::context_t &ctx,  
		ConnectionInfo &ci,
		const std::string host_address,
		const CallbackList cb_list) 
	{
		printf("start_background_connection bind\n");
		zmq::socket_t sock_router(ctx, zmq::socket_type::router);
		connect_monitor_t monitor_router(sock_router, host_address);

		auto send_signal_list = [&](const std::string &identity, ServerMessageType smt, const std::vector<uint8_t> &list) {
			zmq::multipart_t mq(identity);

			flexbuffers::Builder builder;
			builder.Int(smt);
			builder.Finish();

			mq.addmem(builder.GetBuffer().data(), builder.GetBuffer().size());
			mq.addmem(list.data(), list.size());
			auto res = zmq::send_multipart(sock_router, mq, zmq::send_flags::dontwait);
		};


		while (!ci.get_close()) 
		{

			// router disconnected. await for reconnection of zmq
			if (!monitor_router.check_events())
			{
				int reconection_timeout = 300;
				while (true){
					if (ci.get_close())
					{
						break;
					}

					try {
						sock_router.bind(host_address);
						reconection_timeout = 300;
						break;
					}
					catch(zmq::error_t err) {

						monitor_router.check_events();		
						printf("failed: %s. Retry bind...\n", err.what());
						std::this_thread::sleep_for(std::chrono::milliseconds(reconection_timeout));
						reconection_timeout = std::min(5000, reconection_timeout*2);
					}
				}
			}


			process_message(sock_router, ci, cb_list);


			ci.sub_list(send_signal_list);

			// check heartbeat and disconnect device ?

			
			std::this_thread::sleep_for(std::chrono::microseconds(3000));
			
		}

		//monitor_router.;
		


	}


	static void process_message(
		zmq::socket_t &router,
		ConnectionInfo &ci,
		const CallbackList &cb_list
	)
	{
		std::vector<zmq::message_t> msgs;
		if (!zmq::recv_multipart(router, std::back_inserter(msgs), zmq::recv_flags::dontwait))
		{
			return;
		}

		std::string id = msgs[0].to_string();
		DeviceMessageType dmt = static_cast<DeviceMessageType>(flexbuffers::GetRoot(static_cast<const uint8_t*>(msgs[1].data()), msgs[1].size()).AsInt32());

		bool update = true;

		if (ci.sub_connection(id))
		{
			cb_list.connection_callback(id);
		}

		switch(dmt)
		{
			// always destroy and reconstruct data about this identity
			case dmt_connection:
			{
				ci.open_connection(id);
				cb_list.connection_callback(id);
				break;
			}
			// always destroy data about this identity
			case dmt_disconnection:
			{
				update = false;
				ci.close_connection(id);
				cb_list.disconnection_callback(id);
				break;
			}			

			case dmt_alive:
			{
				break;
			}

			case dmt_specification:
			{
				printf("dmt_specification\n");
				//for(auto &m : msgs) printf("size %d\n", m.size());

				auto root = flexbuffers::GetRoot(static_cast<const uint8_t*>(msgs[2].data()), msgs[2].size()).AsVector();
				cb_list.nametype_callback(id, root);
				break;
			}
			case dmt_signaldata:
			{
				auto root = flexbuffers::GetRoot(static_cast<const uint8_t*>(msgs[2].data()), msgs[2].size()).AsVector();
				cb_list.signal_callback(id, root);
				break;
			}
			case dmt_misc:
			{
				break;
			}
	
		}

		ci.update_alive(id);

		//todo: destroy object
	}


	zmq::context_t ctx;
	std::thread connection_thread;
	ConnectionInfo ci;
};

