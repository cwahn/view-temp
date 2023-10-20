#include <iostream>
#include <chrono>
#include <thread>
#include <mutex>
#include <string>
#include <cstring>
#include <tuple>
#include <condition_variable>

#include <mosquitto.h>
#include "flatbuffers/flexbuffers.h"

#include "efp.hpp"

#include <stdio.h>
// #include <stdlib.h>

// #include <unistd.h>

/*
	signal monitoring:
	user-side : 1 send, 1 receive
	device-side : 1 send, 1 receive

	user send 1byte data which describes signal on/off
	device recives description and packkage signal with thi byte and send
	user matchs own byte and recived byte. if matches, accept or discard

	OS independent encode/decode????


	data_supplier(thread0):
		local buffer[]
		while processing
			buffer.pushback()
			buffer.pushback()
			..

			..					-> done at almost constant time (push_back all candidate signal)
			globaldata.set(buffer) -> globaldata free

	data_trans(thread1):
		local buffer[]

		initial_communication()  // block while something?

		while transprocce
			if get_if_supplied(buffer) 	> globaldata free (rapid or slow)
				wait(1ms)
				continue

			get_signal_mask()
			send(encode(buffer))

			..					-> done at variable time

		trans() may tae lone time and miss many supplied data accordig to network state,
		but processing not hang on trans()..
*/


enum SignalType : int16_t
{
	t_yet = 0, // net specified yet
	t_bool,
	t_float,
	t_double,
	t_int
};



#include "signal_list.h"
thread_local SignalThreadID SIGNAL_THREAD_ID{0};

template <typename T>
using SignalBuffer = efp::Vcq<T, 10>;

// double buffer
static constexpr const int SIGNAL_GLOAB_BUFFER_ELEM_SIZE = sizeof(SignalBuffer<int64_t>);
// static constexpr const int SIGNAL_GLOAB_BUFFER_SIZE = SIGNAL_GLOAB_BUFFER_ELEM_SIZE * SIGNAL_NUM;


template <typename T>
struct type_name;

template <>
struct type_name<bool>
{
	static constexpr const SignalType value = t_bool;
};

template <>
struct type_name<int>
{
	static constexpr const SignalType value = t_int;
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
template <int TN>
class SignalSync
{
public:
	//trread /signal id table?  there's chance that not all signal treate
	SignalSync() : num_thread(num_thread)
	{

	}

	template<typename A>
	class StackSignalTypes
	{
	public:
		StackSignalTypes(SignalThreadID tid, SignalID id, const A & value)
		{
			SIGNAL_TYPE_THRD[id] = std::make_tuple(type_name<decltype(value)>::value, tid);
			//static_cast<efp::Vcq<decltype(VALUE), 10>*>(SIGNAL_GLOAB_BUFFER + ID * sizeof(efp::Vcq<int64_t, 10>)) = ;
		}
	};


	// add unknown signal type to treat yet uninitialized signal

	// This must be called once for all threads which access before this class!!
	void supplier_occupy_buf(SignalThreadID tid) {
		std::unique_lock<std::mutex> sync_lock(m_sync[tid]);  
		m_2buf[tid][sobid].lock();
	}


	void set(SignalThreadID tid)
	{
		std::unique_lock<std::mutex> sync_lock(m_sync[tid]);  

		if(!data_supplied[tid])
		{
			data_supplied[tid] = true;
			BufferID sobid = supp_occupied_buf_id[tid];

			// check network job is done
			if(std::try_lock<std::mutex> other_buf_lock(m_2buf[tid][1 - sobid]) != -1) 
			{
				// now confirmed that we can swap buffer
				m_2buf[tid][sobid].unlock();

				sobid = 1 - sobid;
			}
			
		}
	}



	int16_t get()
	{
		for(int tid = 0; tid < num_thread; ++tid)
		{
			std::unique_lock<std::mutex> sync_lock(m_sync[tid]);

			if(data_supplied[tid])
			{
				// previous sobid state : 1 - sobid
				mapped[tid] = std::try_lock<std::mutex> other_buf_lock(m_2buf[tid][1 - sobid]) != -1;
				if(mapped[tid]) 
				{
					// supplier job wasn't finish, so set() not called. 
					// send reuse flag?
				}
			}
		}
	}

	void release()
	{
		for(int tid = 0; tid < num_thread; ++tid)
		{
			std::unique_lock<std::mutex> sync_lock(m_sync[tid]);
			if (mapped[tid])
			{
				// reset buffers
				m_2buf[tid][sobid].unlock();
				data_supplied[tid] = false;
			}
			
		}
	}
			// // reinit from network thread
			// for ()
			// 	init new buffers

	template<typename T>
	static void push_back(SignalThreadID tid, const T &x)
	{
		static_cast<SignalBuffer<T>*>(SIGNAL_GLOAB_BUFFER + ID * sizeof(efp::Vcq<int64_t, 10>)) -> push_back(x);
	}


private:
	enum BufferID {
		m_buffer0 = 0,
		m_buffer1 = 1
	};

	constexpr const int num_thread = TN;
	std::array<std::mutex, num_thread> m_sync;
	std::array<std::array<std::mutex, 2>, num_thread> m_2buf;

	// current buffer id which occupied by supplier
	std::array<BufferID, num_thread>  supp_occupied_buf_id{m_buffer0};

	std::array<bool, num_thread>  data_supplied{flase};
	
	std::array<bool, num_thread>  mapped{flase};

	// used while set get
	// should not be bit field flag
	std::array<int16_t, num_thread> buffer_switch{false};

	
	// used while each thread's push_back()
	// when get() used while push_back(), push_back() can continously access SIGNAL_BUFFER_SWITCH

	std::array<int16_t, num_thread>  SIGNAL_BUFFER_SWITCH{0};
	// double buffer
	int64_t SIGNAL_GLOAB_BUFFER[2][sizeof(efp::Vcq<int64_t, 10>) * SIGNAL_NUM];

};
*/




//template <int TN>
class SignalSync
{
public:
	//trread /signal id table?  there's chance that not all signal treate
	SignalSync() //: num_thread(num_thread)
	{
		// thread id is index, contain signal ids
		//for (int s = 0; s < SIGNAL_NUM; ++s)
		//	THREAD_SIGNAL_ID[std::get<1>(SIGNAL_TYPE_THRD[s])].push_back( std::get<0>(SIGNAL_TYPE_THRD[s]) );
	}

// mutex not needed. 
	void subscribe_thread(const char *thread_name, SignalThreadID tid)
	{
		SIGNAL_GLOAB_BUFFER[tid].thread_name = std::string(thread_name);
	}

	template<typename A>
	void subscribe_signal_once(SignalThreadID tid, const SignalNameID &snid, SignalType stype, const char *signal_name, const A & value)
	{
		// init once
		// indepent thread access different index
		const auto num = SIGNAL_GLOAB_BUFFER[tid].signal_num();
		if(SIGNAL_GLOAB_BUFFER[tid].SUBSCRIBED_YET[num])
		{
			// todo: use match - enum to remove for ?
			for(int t = 0; t < SIGNAL_THREAD_NUM; ++t)
			{
				if(tid == static_cast<SignalThreadID>(t))
				{
					SIGNAL_GLOAB_BUFFER[tid].subscribe_signal<A>(snid, stype, signal_name);		
					// now signal_num changed		
				}
			}
			SIGNAL_GLOAB_BUFFER[tid].SUBSCRIBED_YET[num] = false;
		}
	}

	template<typename T>
	void push_back(SignalThreadID tid, const SignalNameID &snid, const T &x)
	{
		const auto buf_idx = supp_occupied_buf_id[tid];
		SIGNAL_GLOAB_BUFFER[tid].push_back(buf_idx, snid, x);
		// printf("size %d \n", tmp->size());
	}

	template<typename T>
	auto get_buffer(SignalThreadID tid, SignalID sid)
	{
		const auto buf_idx = static_cast<BufferID>(1 - cons_instructed_buf_id[tid]);
		return SIGNAL_GLOAB_BUFFER[tid].get_buffer<T>(buf_idx, sid);
	}
// mutex not needed.



/*
	notify_ready : 이 버퍼 스왑을 수락  실제 버퍼 스왑과 데이터가 변함    수락 전에는 그 데이터 접근 금지

	check_swap_done : 데이터 접근 전에 스왑 수락을 확인 후 되돌림 미수락시 스왑을 패스(공급 지연, 공급을 대기 x)
	notify_swap : 스왑요청 작성   이다음 셋이 어떤 버퍼를 점유할지 지정
*/
	void notify_ready(SignalThreadID tid)
	{
		std::unique_lock<std::mutex> sync_lock(m_sync[tid]);  
		if(!data_supplied[tid])
		{
			data_supplied[tid] = true;
			if(need_swap[tid]) {
				
				accept_swap[tid] = true;
				supp_occupied_buf_id[tid] = cons_instructed_buf_id[tid];	
			}		
		}
		// printf("set: %d, %d, %d, %d\n", data_supplied[tid], need_swap[tid], accept_swap[tid], supp_occupied_buf_id[tid]);
	}

	// return bitflag which describes each thread's swap is done
	int16_t check_swap_done()
	{
		int16_t result = 0;
		for(int tid = 0; tid < SIGNAL_THREAD_NUM; ++tid)
		{
			std::unique_lock<std::mutex> sync_lock(m_sync[tid]);  
			if(need_swap[tid] && accept_swap[tid])
			{
				need_swap[tid] = false;		
				accept_swap[tid] = false;	
				result |= 1 << tid;
			}
			// printf("check: %d, %d, %d, %d\n", data_supplied[tid], need_swap[tid], accept_swap[tid], static_cast<BufferID>(1 - cons_instructed_buf_id[tid]));
		}
		
		return result;
	}

	void notify_swap()
	{
		for(int tid = 0; tid < SIGNAL_THREAD_NUM; ++tid)
		{
			BufferID cibid = cons_instructed_buf_id[tid];

			std::unique_lock<std::mutex> sync_lock(m_sync[tid]);

			if(!need_swap[tid] && !accept_swap[tid])
			{
				data_supplied[tid] = false;
				need_swap[tid] = true;
				cons_instructed_buf_id[tid] = static_cast<BufferID>(1 - cibid);				
			}
			// printf("swap: %d, %d, %d, %d\n", data_supplied[tid], need_swap[tid], accept_swap[tid], static_cast<BufferID>(1 - cons_instructed_buf_id[tid]));
		}
	}

	// used to check if all thread finished their job at least one cycle
	int16_t is_signal_inited()
	{
		int16_t result = 0;
		for(int tid = 0; tid < SIGNAL_THREAD_NUM; ++tid)
		{
			bool new_data = false;
			{
				std::unique_lock<std::mutex> sync_lock(m_sync[tid]);
				new_data = data_supplied[tid];
			}
			
			result |= new_data ? 1 << tid : 0;
		}

		return result;
	}


	// mutex not needed.
	auto get_signal_num(SignalThreadID tid)
	{
		return SIGNAL_GLOAB_BUFFER[tid].signal_num();
	}

	SignalType get_stype(SignalThreadID tid, SignalID sid)
	{
		return SIGNAL_GLOAB_BUFFER[tid].get_stype(sid);
	}
	std::string get_sname(SignalThreadID tid, SignalID sid)
	{
		return SIGNAL_GLOAB_BUFFER[tid].get_sname(sid);
	}
	std::string get_tname(SignalThreadID tid)
	{
		return SIGNAL_GLOAB_BUFFER[tid].thread_name;
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


	std::array<std::mutex, SIGNAL_THREAD_NUM> m_sync;

	// local
	// current buffer id which occupied by supplier
	std::array<BufferID, SIGNAL_THREAD_NUM>  supp_occupied_buf_id{m_buffer0};

	//shared
	// next buffer id which supplier will use
	std::array<BufferID, SIGNAL_THREAD_NUM>  cons_instructed_buf_id{m_buffer0};
	std::array<bool, SIGNAL_THREAD_NUM>  data_supplied{true};
	std::array<bool, SIGNAL_THREAD_NUM>  need_swap{false};
	std::array<bool, SIGNAL_THREAD_NUM>  accept_swap{false};

	// std::array<std::array<SignalID, SIGNAL_NUM>, THREAD_SIGNAL_ID>  THREAD_SIGNAL_ID;


	// mutex not needed.
	class ThreadBuffer
	{
	public:
		// add new signal to table
		template<typename A>
		void subscribe_signal(const SignalNameID &snid, const SignalType &stype, const char *signal_name)
		{
			// network don't access SIGNAL_GLOAB_BUFFER yet
			const auto len = signal_num();

			SIGNAL_NAME.emplace_back(signal_name);
			SIGNAL_TYPE.push_back(stype);
			NAMEID_TO_SIGNALID[snid] = len;

			// todo: remove unecessary initialization
			signal_2buffer[0].resize(len + 1);
			signal_2buffer[1].resize(len + 1);

			// actual init here
			new (&(signal_2buffer[0][len + 1])) SignalBuffer<A>();
			new (&(signal_2buffer[1][len + 1])) SignalBuffer<A>();	
		}

		template<typename T>
		void push_back(const BufferID &bid, const SignalNameID &snid, const T &x)
		{
			const SignalID index = NAMEID_TO_SIGNALID[snid];
			reinterpret_cast<SignalBuffer<T>*>(&(signal_2buffer[bid][index])) -> push_back(x);
		}
		int signal_num()
		{	
			return SIGNAL_TYPE.size();
		}

		template<typename T>
		auto get_buffer(const BufferID &bid, const SignalID &sid)
		{
			// mutex not needed. as we called SignalSync.check()
			return reinterpret_cast<SignalBuffer<T>*>(&(signal_2buffer[bid][sid]));
		}
		SignalType get_stype(const SignalID &sid)
		{	
			return SIGNAL_TYPE[sid];
		}
		std::string get_sname(const SignalID &sid)
		{	
			return SIGNAL_NAME[sid];
		}

		std::string thread_name{"SIGNAL_INIT_LOG_NOT_CALLED"};
		// std::vector<std::tuple<SignalID, SignalType>> SIGNAL_ID{};
		// type can be differ thread by thread : such as template 
		// so here, not global
		std::array<bool, SIGNAL_NAME_NUM> SUBSCRIBED_YET{true};
		std::vector<std::string> SIGNAL_NAME;
		std::vector<SignalType> SIGNAL_TYPE;

		std::array<SignalID, SIGNAL_NAME_NUM> NAMEID_TO_SIGNALID;
		
	private:
		// !! NEVER access this SignalBuffer<int64_t> directly.
		std::vector<SignalBuffer<int64_t>> signal_2buffer[2]{};
	};

	// used to check SIGNAL_LOG() has been called with (thread_id, signal_id)
	ThreadBuffer SIGNAL_GLOAB_BUFFER[SIGNAL_THREAD_NUM];
};



SignalSync SSYNC;

#define SIGNAL_INIT_LOG_(TID) \
	SIGNAL_THREAD_ID = SignalThreadID::TID;

#define SIGNAL_LOG(ID, VALUE) \
	SSYNC.subscribe_signal_once(SIGNAL_THREAD_ID, SignalNameID::ID, type_name<decltype(VALUE)>::value, #ID, VALUE); \
	SSYNC.push_back(SIGNAL_THREAD_ID, SignalNameID::ID, VALUE); 

#define SIGNAL_SYNC_LOG() \
	SSYNC.notify_ready(SIGNAL_THREAD_ID); 









void pack(flexbuffers::Builder &builder, SignalThreadID tid, SignalID sid)
{

#define FLEXBUFFER(TYPE, type) \
	builder.Vector([&]() \
	{ \
		auto buf = SSYNC.get_buffer<type>(tid, sid); \
		auto size = buf->size(); \
		for (int i = 0; i < size; i++) {\
			const auto a = (*buf).pop_front(); \
			builder.TYPE(a); 	\
			builder.TYPE(a); 	\
			builder.TYPE(a); 	\
			builder.TYPE(a); 	\
			builder.TYPE(a); 	\
			builder.TYPE(a); 	\
			builder.TYPE(a); 	\
			builder.TYPE(a); 	\
			builder.TYPE(a); 	\
			builder.TYPE(a); 	\
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
			FLEXBUFFER(Int, int);
			break;
		}
	}
}



/*
<-
vector
	vector
		thread_name
	vector
		vector
			signal_name
		vector
			signal_type

->
	vector(size == thread_num)
		int32
			signal_bitflag
<-
vector
	Int32
		thread_flag
	vector
		Int32
			signal_flag
		vector
			value

- thet compare thread_flag and signal_flag with their own
*/


// The logic I use is to connect than loop. This makes sense to me, in that until you connect successfully looping serves little purpose. I also use connect_async and set a connect=true flag in the on_connect callback, so that I know when it is possible for loop to do any work.

// I also call disconnect before loop_stop, since I want to let on_disconnect run. Even un_subscribe gets aborted when you do explicit unscribes and call loop_stop to fast for the broker to respond.

// I don't tend to call wait_for_publish the way my code is designed, I use the on_publish call back to let my main loop know a publish was completed based on broker response.

class Client
{
public:

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
				// wait for SIGNAL_TYPE_THRD filling
		int16_t first = 0;
		while(first == 0) {
			first |= SSYNC.is_signal_inited();
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		printf("SIGNAL_TYPE_THRD filled  \n");


		mosquitto_lib_init();


		mosq = mosquitto_new("d0", true, (void *)&conn);
		if (mosq == NULL)
		{
			fprintf(stderr, "Error: Out of memory.\n");
			abort();
		}

		flexbuffers::Builder builder;
		builder.Int(Connection::Announce::ann_off);
		builder.Finish();
		auto K = builder.GetBuffer();
		mosquitto_connect_callback_set(mosq, on_connect);
		mosquitto_message_callback_set(mosq, on_message);
		mosquitto_will_set(mosq, "d0/da", K.size(), K.data(), 2, false); 
					

		int rc;

		/* Connect to test.mosquitto.org on port 1883, with a keepalive of 60 seconds.
		 * This call makes the socket connection only, it does not complete the MQTT
		 * CONNECT/CONNACK flow, you should use mosquitto_loop_start() or
		 * mosquitto_loop_forever() for processing net traffic. */
		rc = mosquitto_connect(mosq, "192.168.0.7", 9884, 60);
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


		auto dataRate = std::chrono::microseconds(7000);

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
		//auto result = SSYNC.get();

		//auto result = SSYNC.get();

		auto buffer_ready = SSYNC.check_swap_done();

		// todo: copy or mutex?
		const std::array<int32_t, SIGNAL_THREAD_NUM> out_flag = conn.get_type();
		

		flexbuffers::Builder builder;
		builder.Vector([&]() {

			builder.Int(buffer_ready);

			for(int tid = 0; tid < SIGNAL_THREAD_NUM; ++tid)
			{
				const auto signal_out_flag = out_flag[tid];

				if (!	(buffer_ready & 1 << tid && signal_out_flag != 0)) 
					continue;


				const auto sig_num = SSYNC.get_signal_num(static_cast<SignalThreadID>(tid));
				for(int sid = 0; sid < sig_num; ++sid)
				{
					if (signal_out_flag & 1 << sid)
					{
						pack(builder, static_cast<SignalThreadID>(tid), static_cast<SignalID>(sid));
					}
				}
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
		Connection &connn = *(Connection *)obj;
		/* Print out the connection result. mosquitto_connack_string() produces an
		* appropriate string for MQTT v3.x clients, the equivalent for MQTT v5.0
		* clients is mosquitto_reason_string().
		*/
		printf("on_connect: %s\n", mosquitto_connack_string(reason_code));
		if (reason_code != 0)
		{
			/* If the connection fails for any reason, we don't want to keep on
			* retrying in this example, so disconnect. Without this, the client
			* will attempt to reconnect. */
			// mosquitto_disconnect(mosq);
		}

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
				/*
					next state should be 
						sebd signal list
						or
						Connection::Announce  (because of client side reason)

					other sate should be ignored ??
				*/

				// case client off: ?
			}

			connn.publish_list_type(mosq);
		}
		else if (strcmp(msg->topic, (connn.device_id + "/cslist").c_str()) == 0)
		{
			printf("client sent signal list.\n");
			if (connn.cp_advance_done()) {
				auto signal_flagv = flexbuffers::GetRoot(received_buffer).AsVector();

				std::array<int32_t, SIGNAL_THREAD_NUM> signal_flag;

				for (size_t sf = 0; sf < SIGNAL_THREAD_NUM; ++sf) {
					signal_flag[sf] = signal_flagv[sf].AsInt32();
					// printf("%d", (bool)(sid & 1 << j));
				}
				// printf("\n");
				
				connn.set_type(signal_flag);
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



		void set_type(const std::array<int32_t, SIGNAL_THREAD_NUM> &rec_type)
		{
			mt.set(rec_type);
		}

		std::array<int32_t, SIGNAL_THREAD_NUM> get_type()
		{
			return mt.get();
		}
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


			builder.Vector([&]() {

				// name of thread
				builder.Vector([&]()
				{
					for(int tid = 0; tid < SIGNAL_THREAD_NUM; ++tid)
					{
						builder.String(SSYNC.get_tname(static_cast<SignalThreadID>(tid)));
					}
				});

				// vector of signal
				builder.Vector([&]()
				{
					for(int tid = 0; tid < SIGNAL_THREAD_NUM; ++tid)
					{
						const auto len = SSYNC.get_signal_num(static_cast<SignalThreadID>(tid));
						// vector of s_sname
						builder.Vector([&]()
						{
							for(int sid = 0; sid < len; ++sid)
							{
								builder.String(SSYNC.get_sname(static_cast<SignalThreadID>(tid), static_cast<SignalID>(sid)));
							}
						});
						// vector 0f s_type
						builder.Vector([&]()
						{
							for(int sid = 0; sid < len; ++sid)
							{
								builder.Int(SSYNC.get_stype(static_cast<SignalThreadID>(tid), static_cast<SignalID>(sid)));
							}
						});
					}
				});

				/*
					!!! THIS MAY BE UNSAFE READ !!!
				*/
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
				std::lock_guard<std::mutex> mm(m);
				return receiv;
			}
			void set(const std::array<int32_t, SIGNAL_THREAD_NUM> &rec_type)
			{
				std::lock_guard<std::mutex> mm(m);
				signal_flag = rec_type;
				receiv = true;
			}
			std::array<int32_t, SIGNAL_THREAD_NUM> get()
			{
				std::lock_guard<std::mutex> mm(m);
				return signal_flag;
			}
		private:

			std::mutex m;
			std::array<int32_t, SIGNAL_THREAD_NUM> signal_flag{0};
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

	while (true)
	{
		
	//	SIGNAL_INIT_LOG();

		
		SIGNAL_LOG(wefn, 1);
		buffer[0] = (buffer[0]+1) % 120;
		// printf("%d ", buffer[0]);
		SIGNAL_LOG(dvweb, int(8 * sin((++aa)*0.01)));
		SIGNAL_LOG(greht, 2.0f);

		SIGNAL_LOG(fewfg, -120);
		
		SIGNAL_LOG(fsdfsf, 543);
		std::this_thread::sleep_for(std::chrono::microseconds(1000));
		SIGNAL_SYNC_LOG();

		
	}
}

int mainX()
{
	/* code */

	std::thread S(supl);
	std::thread W(con);

	S.join();
	W.join();
	return 0;
}
