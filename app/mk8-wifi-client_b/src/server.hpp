#include <iostream>
#include <chrono>
#include <thread>
#include <mutex>
#include <string>
#include <cstring>


#include <unordered_map>
#include <unordered_set>
#include <vector>




#include "mosquitto.h"
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


class Client
{
public:


	Client(std::string device_id_) :
		conn(device_id_)
	{
			
	}
	~Client()
	{
		mosquitto_lib_cleanup();
	}

	void init()
	{
		// wait for subscribe_thread()
		int16_t first = 0;
		while(first == 0) {
			first |= SSYNC.is_signal_inited();
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		}

		printf("at least one subscribe_thread filled  \n");


		mosquitto_lib_init();


		mosq = mosquitto_new("d0", true, (void *)&conn);
		if (mosq == NULL)
		{
			fprintf(stderr, "Error: Out of memory.\n");
			abort();
		}


		mosquitto_connect_callback_set(mosq, on_connect);
		mosquitto_message_callback_set(mosq, on_message);
		{
			flexbuffers::Builder builder;
			builder.Int(Connection::Announce::ann_off);
			builder.Finish();
			auto K = builder.GetBuffer();
			mosquitto_will_set(mosq, "d0/da", K.size(), K.data(), 2, false); 
		}
					

		int rc;

		rc = mosquitto_connect(mosq, "192.168.0.16", 9884, 60);
		if (rc != MOSQ_ERR_SUCCESS)
		{
			mosquitto_destroy(mosq);
			fprintf(stderr, "Error: %s\n", mosquitto_strerror(rc));
		}

		/* Run the network loop in a background thread, this call returns quickly. */
		rc = mosquitto_loop_start(mosq);
		if (rc != MOSQ_ERR_SUCCESS)
		{
			mosquitto_destroy(mosq);
			fprintf(stderr, "Error: %s\n", mosquitto_strerror(rc));
		}


		auto dataRate = std::chrono::microseconds(8000);

		auto s0 = std::chrono::high_resolution_clock::now();

		// std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
		int a = 0;

		while (true)
		{
			auto start = std::chrono::high_resolution_clock::now();

			
			if(conn.is_ready()) {
				publish_sensor_data(mosq);
				// ++a;
				// auto d = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - s0);
				// if (d > std::chrono::milliseconds(1000))
				// {
				// 	printf("\n\n\n\n done %d\n", a);
				// 	std::this_thread::sleep_for(std::chrono::milliseconds(100));
				// 	break;
				// }
			}


			auto dt = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start);
			if (dataRate > dt)
			{
				std::this_thread::sleep_for(dataRate - dt);
			}
		}
	}


private:

	void publish_sensor_data(struct mosquitto *mosq)
	{
		int rc;

		int16_t buffer_ready = SSYNC.check_swap_done();

		// todo: copy or mutex?
		const std::vector<std::vector<SignalID>> out_flag = conn.get_type();
		
	

		int16_t out_bitflag = 0;
		for (auto t = 0; t < out_flag.size(); ++t) 
		{
			out_bitflag |= out_flag[t].size() > 0 ? 1<<t : 0;
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


		// although SignalID always transmitted together, but actual load is small
		flexbuffers::Builder builder;
		builder.Vector([&]() {

			builder.Int(out_bitflag);

			// const auto rq_thread_num = std::bitset<16>(out_bitflag).count();
			
			for(SignalThreadID tid = 0; tid < out_flag.size(); ++tid)
			{
				if (!(out_bitflag & 1<<tid))continue;

				const auto sig_num = SSYNC.signal_num(tid);
				builder.Vector([&]() {

					builder.Vector([&]() {
						
						for(SignalID sid : out_flag[tid])
						{
							builder.Int(sid);
							//printf("%d, ", sid);

						}//printf("\n");
					});

					builder.Vector([&]() {
						for(SignalID sid : out_flag[tid])
						{
							pack(builder, tid, sid);
						}
					});
				});
			}
		});
		builder.Finish();
		auto K = builder.GetBuffer();



			//for(int sid = 0; sid < SIGNAL_NUM; ++sid)
			// {
			// 	int sid = 1;
			// 	auto tid = SIGNAL_THREAD[sid];
			// 	if (result & 1 << tid)// && out_flag & 1 << sid)
			// 	{
			// 		auto buf = SSYNC.get_buffer<int>(tid, static_cast<SignalID>(sid));
			// 		for(int i = 0; i < buf->size(); ++i)
			// 		{
			// 			auto A = (*buf).pop_front(); 	

			// 			printf("%d, ", A);
			// 		}
			// 		printf("\n");
			// 		// SSYNC.reset_buffer<int>(tid, static_cast<SignalID>(sid));
			// 	}
			// }



		// auto buf = SSYNC.get_buffer<type>(tid, sid); 
		//  for (int i = 0; i < 1; i++)
		// 	builder.TYPE((*buf).pop_front()); 	


		SSYNC.notify_swap();

		rc = mosquitto_publish(mosq, NULL, conn.device_id.c_str(), K.size(), K.data(), 0, false);
		if (rc != MOSQ_ERR_SUCCESS)
		{
			fprintf(stderr, "Error publishing: %s\n", mosquitto_strerror(rc));
		}

	}



	/* Callback called when the client receives a CONNACK message from the broker. */
	static void on_connect(struct mosquitto *mosq, void *obj, int reason_code)
	{
		printf("on_connect: %s\n", mosquitto_connack_string(reason_code));
		if (reason_code != 0)
		{
			/* If the connection fails for any reason, we don't want to keep on
			* retrying in this example, so disconnect. Without this, the client
			* will attempt to reconnect. */
			// mosquitto_disconnect(mosq);
			return;
		}
		
		Connection &connn = *(Connection *)obj;

		int rc;
		rc = mosquitto_subscribe(mosq, NULL, (connn.device_id + "/ca").c_str(), 2);
		if (rc != MOSQ_ERR_SUCCESS)
		{
			fprintf(stderr, "Error subscribing: %s\n", mosquitto_strerror(rc));
			/* We might as well disconnect if we were unable to subscribe */
			mosquitto_disconnect(mosq);
		}
		rc = mosquitto_subscribe(mosq, NULL, (connn.device_id + "/cslist").c_str(), 2);
		if (rc != MOSQ_ERR_SUCCESS)
		{
			fprintf(stderr, "Error subscribing: %s\n", mosquitto_strerror(rc));
			/* We might as well disconnect if we were unable to subscribe */
			mosquitto_disconnect(mosq);
		}

		connn.publish_announce(mosq, Connection::Announce::ann_on);
		/* You may wish to set a flag here to indicate to your application that the
		* client is now connected. */
	}

	/* Callback called when the client knows to the best of its abilities that a
	* PUBLISH has been successfully sent. For QoS 0 this means the message has
	* been completely written to the operating system. For QoS 1 this means we
	* have received a PUBACK from the broker. For QoS 2 this means we have
	* received a PUBCOMP from the broker. */
	static void on_publish(struct mosquitto *mosq, void *obj, int mid)
	{
		printf("Message with mid %d has been published.\n", mid);
	}

	static void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
	{
		std::vector<uint8_t> received_buffer(static_cast<uint8_t*>(msg->payload), static_cast<uint8_t*>(msg->payload) + msg->payloadlen);
		Connection &connn = *(Connection *)obj;

		if (strcmp(msg->topic, (connn.device_id + "/ca").c_str()) == 0)
		{
			auto ann = static_cast<typename Connection::Announce>(flexbuffers::GetRoot(received_buffer).AsInt32());
			switch(ann)
			{
				case Connection::Announce::ann_on:
				{
					printf("client announce : ann_on\n");
					// reset all data!!!
					//
					connn.publish_announce(mosq, Connection::Announce::ann_reply);
					connn.cp_advance_done();
					break;
					// if device disconnected here ? 
				}
				case Connection::Announce::ann_reply:
				{
					printf("client announce : ann_reply\n");
					connn.cp_advance_done();
					break;
				}
				case Connection::Announce::ann_off:
				{
					printf("deviclientce announce : ann_off\n");
					connn.cp_reset();
					break;
				}
				/*
					next state should be 
						sebd signal list
						or
						Connection::Announce  (because of client side reason)

					other sate should be ignored ??
				*/

			}

			connn.publish_list_type(mosq);
		}
		else if (strcmp(msg->topic, (connn.device_id + "/cslist").c_str()) == 0)
		{
			printf("client sent signal list.\n");
			if (connn.cp_advance_done()) {
				auto root = flexbuffers::GetRoot(received_buffer).AsVector();
				const auto tlen = root.size();

				std::vector<std::vector<SignalID>> thread_signal_list(tlen);
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
				// printf("\n");
				
				connn.set_type(thread_signal_list);
			}
		}
	}


	class Connection
	{
	public:
		Connection(std::string device_id_) :
			device_id(device_id_)
		{

				
		}
		enum Announce : int8_t 
		{
			ann_on,
			ann_reply,
			ann_off

		};
	
		enum ConnPhase {
			cp_wait_response,
			cp_done,
			done
		};
	
		void cp_reset()
		{
			cp = cp_wait_response;
			mt.reset();
			// other reset work.
		}
		bool cp_advance_done()
		{
			if(cp == cp_wait_response || cp_done)
			{
				cp = cp_done;
				return true;
			}
			return false;
		}
		bool cp_is_done()
		{
			return cp == cp_done;
		}



		void set_type(const std::vector<std::vector<SignalID>>&rec_type)
		{
			mt.set(rec_type);
		}

		std::vector<std::vector<SignalID>> get_type()
		{
			return mt.get();
		}

		// received client sent signal list 
		bool is_ready() 
		{
			return mt.is_ready();
		}

		void publish_announce(struct mosquitto *mosq, Announce ann)
		{
			flexbuffers::Builder builder;
			builder.Int(ann);
			builder.Finish();
			auto K = builder.GetBuffer();

			int rc = mosquitto_publish(mosq, NULL, (device_id + "/da").c_str(), K.size(), K.data(), 2, false);
			if (rc != MOSQ_ERR_SUCCESS)
			{
				fprintf(stderr, "Error publishing: %s\n", mosquitto_strerror(rc));
			}
		}

		void publish_list_type(struct mosquitto *mosq)
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
			auto K = builder.GetBuffer();

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



			int rc = mosquitto_publish(mosq, NULL, (device_id + "/slist").c_str(), K.size(), K.data(), 2, false);
			if (rc != MOSQ_ERR_SUCCESS)
			{
				fprintf(stderr, "Error publishing: %s\n", mosquitto_strerror(rc));
			}
		}

		const std::string device_id;


	private:

		class MessageType 
		{
		public:
			bool is_ready()
			{
				std::lock_guard<std::mutex> m_lock(m);
				return receiv;
			}
			void set(const std::vector<std::vector<SignalID>> &rec_type)
			{
				std::lock_guard<std::mutex> m_lock(m);
				thread_signal_list = rec_type;
				receiv = true;
			}
			std::vector<std::vector<SignalID>> get()
			{
				std::lock_guard<std::mutex> m_lock(m);
				return thread_signal_list;
			}

			void reset()
			{
				std::lock_guard<std::mutex> m_lock(m);
				receiv = false;
				thread_signal_list.clear();
			}

		private:

			std::mutex m;
			std::vector<std::vector<SignalID>> thread_signal_list;
			bool receiv {false};
		};

		MessageType mt;
		ConnPhase cp{cp_wait_response};
	};

	struct mosquitto *mosq;
	Connection conn;
};














void con()
{
	Client x(std::string("d0"));
	x.init();
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

	S.join();
	//S2.join();
	W.join();




	
	return 0;
}
