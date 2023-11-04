#include <iostream>
#include <chrono>
#include <thread>
#include <mutex>
#include <string>
#include <cstring>


#include <unordered_map>
#include <unordered_set>
#include <vector>



#include "zmq.hpp"
#include "zmq_addon.hpp"
#include "flatbuffers/flexbuffers.h"

#include "efp.hpp"

#include <stdio.h>


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

template <typename T>
using SignalBuffer = efp::Vcq<T, 10>;

// template <typename T>
// struct alignas(SignalBuffer<T>) SignalBufferAlign {
// 	SignalBuffer<T> data{};
// };


// using SignalBufferAlign = std::array<int8_t, sizeof(SignalBuffer<int64_t>)>;


class alignas(SignalBuffer<int64_t>) SignalBufferAlign {
	public:
	int8_t data[sizeof(SignalBuffer<int64_t>)];
};

template <typename T>
struct type_name;

template <>
struct type_name<bool>
{
	static constexpr const SignalType value = t_bool;
};

template <>
struct type_name<int32_t>
{
	static constexpr const SignalType value = t_int;
};

template <>
struct type_name<int64_t>
{
	static constexpr const SignalType value = t_int64;
};

template <>
struct type_name<float>
{
	static constexpr const SignalType value = t_float;
};

template <>
struct type_name<double>
{
	static constexpr const SignalType value = t_double;
};



/*
	todo: move this into SignalSync's private
	static thread_local SignalThreadID thread_id; at private cause long error.
*/
thread_local SignalThreadID thread_id;



/*
todo: add flag which notifies new thread or signal subscribed
	subscribe_thread
	subscribe_signal_and_push_back


	staticassert when elem size > 64

*/


class SignalSync
{
public:
	SignalSync() 
	{
	}

	void subscribe_thread(const char *thread_name)
	{
		std::unique_lock<std::mutex> thread_lock(m_thread_info);
		if (thread_table.find(std::this_thread::get_id()) == thread_table.end()) {
			// subscribe thread id
			thread_id = thread_table.size();
			thread_table.insert(std::this_thread::get_id());
			
			// init thread specific storage
			SIGNAL_GLOAB_BUFFER.emplace_back(std::make_unique<ThreadBuffer>());
			SIGNAL_GLOAB_BUFFER[thread_id]->thread_name = std::string(thread_name);
			printf("subscribe_thread %s %d\n", SIGNAL_GLOAB_BUFFER[thread_id]->thread_name.c_str(), thread_id);
		}
		// else
		// error. same thread initialized!!
	}

// mutex not needed. 

	// for supplier
	template<typename A>
	void subscribe_signal_and_push_back(const std::string &signal_name, const A &x)
	{
		auto &sgb = *SIGNAL_GLOAB_BUFFER[thread_id];
		auto iter = sgb.signal_table.find(signal_name); 

		if (iter == sgb.signal_table.end())
		{
			iter = sgb.subscribe_signal<A>(signal_name);
			printf("subss %s, %d\n", signal_name.c_str(), iter->second);
		}

		sgb.push_back<A>(iter->second, x);
	}

	// for consumer
	template<typename T>
	auto get_buffer(const SignalThreadID &tid, const SignalID &sid)
	{
		auto &sgb = *SIGNAL_GLOAB_BUFFER[tid];

		return sgb.get_buffer<T>(sid);
	}

// mutex not needed.



/*
	notify_ready : 이 버퍼 스왑을 수락  실제 버퍼 스왑과 데이터가 변함    수락 전에는 그 데이터 접근 금지

	check_swap_done : 데이터 접근 전에 스왑 수락을 확인 후 되돌림 미수락시 스왑을 패스(공급 지연, 공급을 대기 x)
	notify_swap : 스왑요청 작성   이다음 셋이 어떤 버퍼를 점유할지 지정
*/
	// for supplier
	void notify_ready()
	{
		auto &sgb = *SIGNAL_GLOAB_BUFFER[thread_id];
		std::unique_lock<std::mutex> sync_lock(sgb.m_sync);  
		if(!sgb.data_supplied)
		{
			sgb.data_supplied = true;
			if(sgb.need_swap) {
				
				sgb.accept_swap = true;
				sgb.supp_occupied_buf_id = sgb.cons_instructed_buf_id;	
			}		
		}
		// printf("set: %d, %d, %d, %d\n", data_supplied[tid], need_swap[tid], accept_swap[tid], supp_occupied_buf_id[tid]);
	}
	// for consumer
	// return bitflag which describes each thread's swap is done
	int16_t check_swap_done()
	{
		int16_t result = 0;
		const SignalThreadID thread_num_ = thread_num();
		for(SignalThreadID tid = 0; tid < thread_num_; ++tid)
		{
			auto &sgb = *SIGNAL_GLOAB_BUFFER[tid];
			std::unique_lock<std::mutex> sync_lock(sgb.m_sync);  

			if(sgb.need_swap && sgb.accept_swap)
			{
				sgb.need_swap = false;		
				sgb.accept_swap = false;	
				result |= 1 << tid;
			}
			// printf("check: %d, %d, %d, %d\n", data_supplied[tid], need_swap[tid], accept_swap[tid], static_cast<BufferID>(1 - cons_instructed_buf_id[tid]));
		}
		
		return result;
	}
	// for consumer
	void notify_swap()
	{
		const SignalThreadID thread_num_ = thread_num();
		for(SignalThreadID tid = 0; tid < thread_num_; ++tid)
		{
			auto &sgb = *SIGNAL_GLOAB_BUFFER[tid];
			// safe
			BufferID cibid = sgb.cons_instructed_buf_id;

			std::unique_lock<std::mutex> sync_lock(sgb.m_sync);

			if(!sgb.need_swap && !sgb.accept_swap)
			{
				sgb.data_supplied = false;
				sgb.need_swap = true;
				sgb.cons_instructed_buf_id = static_cast<BufferID>(1 - cibid);				
			}
			// printf("swap: %d, %d, %d, %d\n", data_supplied[tid], need_swap[tid], accept_swap[tid], static_cast<BufferID>(1 - cons_instructed_buf_id[tid]));
		}
	}

	// used to check if all thread finished their job at least one cycle
	int16_t is_signal_inited()
	{
		int16_t result = 0;
		const SignalThreadID thread_num_ = thread_num();
		for(SignalThreadID tid = 0; tid < thread_num_; ++tid)
		{
			bool new_data = false;
			{	auto &sgb = *SIGNAL_GLOAB_BUFFER[tid];

				std::unique_lock<std::mutex> sync_lock(sgb.m_sync);
				new_data = sgb.data_supplied;
			}
			
			result |= new_data ? 1 << tid : 0;
		}

		return result;
	}

	SignalThreadID thread_num()
	{
		std::unique_lock<std::mutex> sync_lock(m_thread_info);
		return SIGNAL_GLOAB_BUFFER.size();
	}

	/*
		todo: fix unsafe access
		THIS MAY BE UNSAFE READ
		while supplier subscribe new signal / comsumer read value
		atomic count?
	*/
	auto signal_num(const SignalThreadID &tid)
	{
		return SIGNAL_GLOAB_BUFFER[tid]->signal_num();
	}

	// mutex not needed.
	std::string get_sname(const SignalThreadID &tid, const SignalID &sid)
	{
		return SIGNAL_GLOAB_BUFFER[tid]->get_sname(sid);
	}
	SignalType get_stype(const SignalThreadID &tid, const SignalID &sid)
	{
		return SIGNAL_GLOAB_BUFFER[tid]->get_stype(sid);
	}
	
	std::string get_tname(const SignalThreadID &tid)
	{
		return SIGNAL_GLOAB_BUFFER[tid]->thread_name;
	}
	// template<typename F>
	// void foreach_signal(SignalThreadID tid, const F &f)
	// {
	// 	const auto len = SIGNAL_GLOAB_BUFFER[tid].signal_num();
	// 	for(SignalID sid = 0; sid < len; ++sid)
	// 	{
	// 		f();
	// 	}
	// }


private:

	enum BufferID {
		m_buffer0 = 0,
		m_buffer1 = 1
	};

	

	class ThreadBuffer
	{
	public:
		~ThreadBuffer() 
		{
			for(auto &x : signal_2buffer[0])
			{
				if (x)
					delete[] x;
			}

			for(auto &x : signal_2buffer[1])
			{
				if (x)
					delete[] x;
			}
		}

		// for supplier
		// add new signal to table
		template<typename T>
		auto subscribe_signal(const std::string &signal_name)
		{
			// network don't access SIGNAL_GLOAB_BUFFER yet ? not !! ???? todo: handle this?
			const auto len = signal_table.size();
			
			const auto table_iter = signal_table.insert({signal_name, len}).first;

			sname_table.emplace(len, signal_name);

			signal_type.push_back(type_name<T>::value);

			// actual init here
			signal_2buffer[0].push_back(reinterpret_cast<int8_t*>(new SignalBuffer<T>()));
			signal_2buffer[1].push_back(reinterpret_cast<int8_t*>(new SignalBuffer<T>()));

			return table_iter;
		}

		// for supplier
		template<typename T>
		void push_back(const SignalID &sid, const T &x)
		{
			reinterpret_cast<SignalBuffer<T>*>(signal_2buffer[supp_occupied_buf_id][sid]) -> push_back(x);
		}

		// for both
		int signal_num()
		{	
			return signal_table.size();
		}

		// for consumer
		template<typename T>
		auto get_buffer(const SignalID &sid)
		{
			// mutex not needed. as we called SignalSync.check()
			const auto bid = static_cast<BufferID>(1 - cons_instructed_buf_id);
			return reinterpret_cast<SignalBuffer<T>*>(signal_2buffer[bid][sid]);
		}

		SignalType get_stype(const SignalID &sid)
		{	
			return signal_type[sid];
		}
		std::string get_sname(const SignalID &sid)
		{	
			return sname_table[sid];
		}

	// mutex not needed. indirectly handled by m_sync.
		std::string thread_name{"SIGNAL_INIT_LOG_NOT_CALLED"};
	
		// SignalID is nickname of std::string(signal name)
		std::unordered_map<std::string, SignalID> signal_table;

		// to access signal name from signal ID 
		// todo: waste large memoey, find better way
		std::unordered_map<SignalID, std::string> sname_table;

		// type can be differ thread by thread : such as template 
		std::vector<SignalType> signal_type;
		// !! NEVER access this SignalBufferAlign<int64_t> directly.
		std::vector<int8_t*> signal_2buffer[2];


	//  extra data for syncroization
		std::mutex m_sync;
		// local of supplier
		// current buffer id which occupied by supplier
		BufferID  supp_occupied_buf_id{m_buffer0};
		// shared between supplier and consumer
		// next buffer id set by consumer and supplier will use this id later
		BufferID  cons_instructed_buf_id{m_buffer0};
		bool  data_supplied{true};
		bool  need_swap{false};
		bool  accept_swap{false};

	private:

	};


	std::mutex m_thread_info;
	std::unordered_set<std::thread::id> thread_table;
	
	// requested uotput signal list?
	std::vector<std::unordered_set<SignalID>> rq_signal_list;

	std::vector<std::unique_ptr<ThreadBuffer>> SIGNAL_GLOAB_BUFFER;
};

/*
SignalNameID : when push_back, where to save value?  str compare of id table...
	todo: replace this with real signal id string 
*/
static SignalSync SSYNC;

#define SIGNAL_INIT_LOG_(TNAME) \
	SSYNC.subscribe_thread(#TNAME);

#define SIGNAL_LOG(SNAME, VALUE) \
	SSYNC.subscribe_signal_and_push_back(#SNAME, VALUE); 

#define SIGNAL_SYNC_LOG() \
	SSYNC.notify_ready(); 







void pack(flexbuffers::Builder &builder, SignalThreadID tid, SignalID sid)
{

#define FLEXBUFFER(TYPE, type) \
	builder.Vector([&]() \
	{ \
		auto buf = SSYNC.get_buffer<type>(tid, sid); \
		auto size = buf->size(); \
		for (int i = 0; i < size; i++) {\
			const type a = buf->pop_front(); \
			builder.TYPE(a); \
		} \
	});

//			printf("%d, ", a); printf("\n"); 

	SignalType stype = SSYNC.get_stype(tid, sid);

	switch (stype)
	{
		case t_bool:
		{
			FLEXBUFFER(Bool, bool);
			break;
		}
		case t_float:
		{
			FLEXBUFFER(Float, float);
			break;
		}
		case t_double:
		{
			FLEXBUFFER(Double, double);
			break;
		}
		case t_int:
		{
			FLEXBUFFER(Int, int32_t);
			break;
		}
		case t_int64:
		{
			FLEXBUFFER(Int, int64_t);
			break;
		}
	}
}



/*

seperate fullsize or requested size

<-

	vector(thread_num)
		string
			thread_name
		//int32
		//	thread_id 	<- id is index of this vector 
		vector
			signal_name
		vector
			signal_type
		//vector
		//	signal_id 	<- id is index of this vector 

->

	vector(thread_num)
		vector
			int32
				signal_id

<-

	Int32
		thread_flag
	vector(requested and ready thread_num, can be empty size ? or ? fill with default value?)
		vector(requested signal_num)
			Int32
				signal_id
		vector(requested signal_num)
			vector
				value

- thet compare thread_flag and signal_flag with their own
*/

/*
publish:
	d0 - signal
	d0/da - device connection ON/OFF    deviceAnnounce
	d0/slist - list of available signal

subscribe:
	d0/ca - client ID			clientAnnounce
	d0/cslist - signal IDsto send  (multiple client? ID0 | ID1 | ... )


device ON
	start supplier thread
	start network thread
		init lib, connect and subscribe
		publish: d0/stat as ON
		wait for signal table manipulatin  {}  //assume this table is complete table and no further changes occur

		wait for d0/clid {}
		publish: d0/slist
		wait for d0/cslist {}
		var ready = d0/clid d0/slist d0/cslist  done

		while (true)
			while (!ready)
				// handle no client status
				wait for d0/clid {}
				publish: d0/slist
				wait for d0/cslist {}

			get()
			publish: d0


assume there be supplied data

*/


// todo: how to stop?
class Client
{
public:
	Client(std::string server_address) :
		ctx{},
		server_address{server_address}
	{
	}
	~Client()
	{
		
	}

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
			connection = true;
			std::cout << "on_event_connected " << addr << std::endl;
		}
		void on_event_connect_delayed(const zmq_event_t& event,
								const char* addr) override
		{
			//std::cout << "on_event_connect_delayed " << addr << std::endl;
		}		
		void on_event_connect_retried(const zmq_event_t& event,
								const char* addr) override
		{
			//std::cout << "on_event_connect_retried " << addr << std::endl;
		}
		void on_event_listening(const zmq_event_t& event,
								const char* addr) override
		{
			//connection = true;
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
			//connection = true;
			std::cout << "on_event_accepted " << addr << std::endl;
		}
		void on_event_accept_failed(const zmq_event_t& event,
								const char* addr) override
		{
			//connection = false;
			std::cout << "on_event_accept_failed " << addr << std::endl;
		}
		void on_event_closed(const zmq_event_t& event,
								const char* addr) override
		{
			//connection = false;
			//std::cout << "on_event_closed " << addr << std::endl;
		}
		void on_event_close_failed(const zmq_event_t& event,
								const char* addr) override
		{
			std::cout << "on_event_close_failed " << addr << std::endl;
		}
		void on_event_disconnected(const zmq_event_t& event,
								const char* addr) override
		{
			connection = false;
			std::cout << "on_event_disconnected " << addr << std::endl;
		}

		bool check_events()
		{
			for(int i = 0; i > -1; ++i) 
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



	void operator()() {

		int16_t first = 0;
		while(first == 0) {
			first |= SSYNC.is_signal_inited();
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		}

		printf("start_background_connection bind\n");
		zmq::socket_t sock_dealer(ctx, zmq::socket_type::dealer);
		sock_dealer.set(zmq::sockopt::routing_id, std::string("device001"));
		sock_dealer.set(zmq::sockopt::reconnect_ivl, 1000);
		sock_dealer.set(zmq::sockopt::reconnect_ivl_max, 5000);

		sock_dealer.set(zmq::sockopt::heartbeat_ivl, 3000);
		sock_dealer.set(zmq::sockopt::heartbeat_timeout, 30000);



		connect_monitor_t monitor_dealer(sock_dealer, server_address);


		auto dataRate = std::chrono::microseconds(8000);
		// auto start = std::chrono::high_resolution_clock::now();

		while (!close_device) 
		{
			auto start = std::chrono::high_resolution_clock::now();

			if (!monitor_dealer.check_events() || cs_at(ConnectionStage::sg_disconnected))
			{
				// reset some data
				
				int reconection_timeout = 1000;

				while (!close_device) {
					// if (ci.get_close())
					// {
					// 	break;
					// }

					try {
						printf("reconnect...\n");
						sock_dealer.connect(server_address);
						cs_reset();
						if (monitor_dealer.check_events())
						{
							cs_advance(ConnectionStage::sg_sent_list);
							publish_list_type(sock_dealer);
							break;
						}
						else
						{
							std::this_thread::sleep_for(std::chrono::milliseconds(reconection_timeout));
							reconection_timeout = std::min(5000, reconection_timeout*2);
						}
					}
					catch(zmq::error_t err) {

						monitor_dealer.check_events();		
						printf("failed: %s. Retry connect...\n", err.what());
						std::this_thread::sleep_for(std::chrono::milliseconds(reconection_timeout));
						reconection_timeout = std::min(5000, reconection_timeout*2);
					}
				}


			}

			
			
			process_message(sock_dealer);

			if (cs_check(ConnectionStage::sg_received_spec))
				publish_sensor_data(sock_dealer);
			
		
			// check heartbeat and disconnect device ?

			
			auto dt = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start);
			if (dataRate > dt)
			{
				std::this_thread::sleep_for(dataRate - dt);
			}
		}

    }

private:
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

	struct ConnectionStage
	{
		static constexpr int sg_disconnected{0};
		static constexpr int sg_sent_list{1};
		static constexpr int sg_received_spec{2};
		static constexpr int sg_send_signal{3};
	};

	void cs_reset()
	{
		cstage = ConnectionStage::sg_disconnected;
		// other reset work.
	}

	bool cs_advance(int cs_adv)
	{
		if(cstage >= std::max(cs_adv-1, ConnectionStage::sg_disconnected))
		{
			cstage = std::max(cstage, cs_adv);
			return true;
		}
		return false;
	}

	bool cs_check(int cs)
	{
		return cstage >= std::max(cs, ConnectionStage::sg_disconnected);
	}
	bool cs_at(int cs)
	{
		return cstage == ConnectionStage::sg_disconnected;
	}
	bool cs_done()
	{
		return cstage == ConnectionStage::sg_send_signal;
	}

	void send_list(zmq::socket_t &dealer, DeviceMessageType dmt, const std::vector<uint8_t> &list) 
	{
		zmq::multipart_t mq;

		flexbuffers::Builder builder;
		builder.Int(dmt);
		builder.Finish();

		mq.addmem(builder.GetBuffer().data(), builder.GetBuffer().size());
		mq.addmem(list.data(), list.size());
		auto res = zmq::send_multipart(dealer, mq, zmq::send_flags::dontwait);
	}


	void process_message(zmq::socket_t &dealer)
	{
		std::vector<zmq::message_t> msgs;
		if (!zmq::recv_multipart(dealer, std::back_inserter(msgs), zmq::recv_flags::dontwait))
		{
			// nothing 
			return;
		}
		
		//std::string id = msgs[0].to_string();
		ServerMessageType smt = static_cast<ServerMessageType>(flexbuffers::GetRoot(static_cast<uint8_t*>(msgs[0].data()), msgs[0].size()).AsInt32());

		bool update = true;

		switch(smt)
		{
			// always destroy and reconstruct data about this identity
			case smt_connection:
			{
				break;
			}
			// always destroy data about this identity
			case smt_disconnection:
			{
				update = false;
				break;
			}			

			case smt_alive:
			{
				break;
			}

			case smt_specification:
			{
				printf("client sent signal list.\n");
				//for(auto &m : msgs) printf("size %d\n", m.size());

				if (cs_advance(ConnectionStage::sg_received_spec)) {
					auto root = flexbuffers::GetRoot(static_cast<const uint8_t*>(msgs[1].data()), msgs[1].size()).AsVector();
					const auto tlen = root.size();

					thread_signal_list.clear();
					thread_signal_list.resize(tlen);
					// FULL SIZED
					for (int tid = 0; tid < tlen; ++tid) {

						const auto f_signal_list = root[tid].AsVector();
						const auto slen = f_signal_list.size();

						auto &signal_list = thread_signal_list[tid];
						signal_list.reserve(slen);
						
						for (int sid = 0; sid < slen; ++sid) {
							signal_list.emplace_back(f_signal_list[sid].AsInt32());
							printf("%d, ", f_signal_list[sid].AsInt32());
						}
						printf("\n");
					}
				}
				break;
			}

			case smt_misc:
			{
				publish_list_type(dealer);
				break;
			}
	
		}

		//todo: destroy object
	}


	void publish_sensor_data(zmq::socket_t &dealer)
	{
		int rc;
		int16_t buffer_ready = SSYNC.check_swap_done();

		int16_t out_bitflag = 0;
		for (auto t = 0; t < thread_signal_list.size(); ++t) 
		{
			out_bitflag |= thread_signal_list[t].size() > 0 ? 1<<t : 0;
			//printf(" %d,", buffer_ready & 1<<t  ? 1 : 0);
			//printf(" %d,", out_flag[t].size());
		}
		 //printf("\n");

		out_bitflag &= buffer_ready;
		
		// for (auto t = 0; t < out_flag.size(); ++t) 
		// 	printf(" %d,", out_bitflag & 1<<t  ? 1 : 0);
		// printf("\n");

		/*		
			out thread = 
			requested thread &&			<- implies that requested signal num in this thread  > 0
			prepared(swaped) thread
		*/

		// although SignalID always transmitted together, actual load is small
		flexbuffers::Builder builder;
		builder.Vector([&]() {

			builder.Int(out_bitflag);

			// const auto rq_thread_num = std::bitset<16>(out_bitflag).count();
			
			for(SignalThreadID tid = 0; tid < thread_signal_list.size(); ++tid)
			{
				if (!(out_bitflag & 1<<tid))continue;

				const auto sig_num = SSYNC.signal_num(tid);
				builder.Vector([&]() {

					builder.Vector([&]() {
						
						for(SignalID sid : thread_signal_list[tid])
						{
							builder.Int(sid);
							//printf("%d, ", sid);

						}//printf("\n");
					});

					builder.Vector([&]() {
						for(SignalID sid : thread_signal_list[tid])
						{
							pack(builder, tid, sid);
						}
					});
				});
			}
		});
		builder.Finish();


		SSYNC.notify_swap();

		send_list(dealer, DeviceMessageType::dmt_signaldata, builder.GetBuffer());
	}

	void publish_list_type(zmq::socket_t &dealer)
	{
		flexbuffers::Builder builder;

		// for(int sid = 0; sid < SIGNAL_NUM; ++sid)
		// {
		// 	std::cout << SIGNAL_NAME[sid] << ", ";
		// }
		// std::cout << std::endl;

		builder.Vector([&]() 
		{
			const auto thread_num = SSYNC.thread_num();
			for(SignalThreadID tid = 0; tid < thread_num; ++tid)
			{
				/*
					!!! THIS MAY BE UNSAFE READ !!!
				*/
				const auto sig_num = SSYNC.signal_num(tid);

				// thread name
				builder.String(SSYNC.get_tname(tid));
				
				// vector of signal name
				builder.Vector([&]() 
				{
					for(int sid = 0; sid < sig_num; ++sid)
					{
						builder.String(SSYNC.get_sname(tid, sid));
					}
				});

				// vector of signal type
				builder.Vector([&]()
				{
					for(int sid = 0; sid < sig_num; ++sid)
					{
						builder.Int(SSYNC.get_stype(tid, sid));
					}
				});
			}

		});

		builder.Finish();
	
		// std::vector<uint8_t> rb(K.data(), K.data() + K.size());
		// 	auto root = flexbuffers::GetRoot(rb).AsVector();
		// 	auto sname = root[0].AsVector();
		// 	auto stype = root[1].AsVector();

		// 	for (size_t j = 0; j < sname.size(); j++) {
		// 		std::cout << sname[j].AsString().str();
		// 	}
		// 	std::cout << std::endl;
			

		// 	for (size_t j = 0; j < stype.size(); j++) {
		// 		std::cout << stype[j].AsInt32();
		// 	}
		// 	std::cout << std::endl;
		printf("dmt_specification size %d\n", builder.GetBuffer().size());
		send_list(dealer, DeviceMessageType::dmt_specification, builder.GetBuffer());
	}



	zmq::context_t ctx;
	std::string server_address;

	// receive specification, reset everytime  
	std::vector<std::vector<SignalID>> thread_signal_list;
	
	// bool received_specification{false};

	bool close_device{false};

	int cstage{ConnectionStage::sg_disconnected};

};















void con()
{
	Client x(std::string("tcp://192.168.0.47:5555"));
	x();
}


void supl()
{
	SIGNAL_INIT_LOG_(Suplier_thread);
	int buffer[16]{0};
	int aa = 0;
	auto start = std::chrono::high_resolution_clock::now();

	while (true)
	{
		

		auto dti = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count();
		auto dt = (float)dti / 1000.0f;
				
		
		SIGNAL_LOG(c1.33, 1.333);

		SIGNAL_LOG(sindt2, int(8 * sin(dt*2.0)));
		SIGNAL_LOG(c2, 23.0f);

		SIGNAL_LOG(tri, int(dti)%500);
		
		SIGNAL_LOG(rect, dti%2000 > 1000 ? 0 : 1);
		std::this_thread::sleep_for(std::chrono::microseconds(1000));
		SIGNAL_SYNC_LOG();
	}
}

void supl2()
{
	SIGNAL_INIT_LOG_(somethread2);
	
auto start = std::chrono::high_resolution_clock::now();

	while (true)
	{

	auto dti = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count();
auto dt = (float)dti / 1000.0f;
			

		SIGNAL_LOG(sin16, sin(dt*5.0));

		SIGNAL_LOG(rec25, int(dti)%250);
		
		std::this_thread::sleep_for(std::chrono::microseconds(4000));
		SIGNAL_SYNC_LOG();

	}
}


int mainX()
{
	/* code */

	std::thread S(supl);
	//std::thread S2(supl2);
	std::thread W(con);


efp::Vector<float> con;
con.push_back(3.0);

float aa = efp::mean<float>(con);


	S.join();
	//S2.join();
	W.join();




	
	return 0;
}
