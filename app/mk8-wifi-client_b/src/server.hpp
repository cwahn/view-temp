#include <iostream>
#include <chrono>
#include <thread>
#include <mutex>
#include <string>
#include <cstring>
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
	static constexpr const int SIGNAL_GLOAB_BUFFER_SIZE = SIGNAL_GLOAB_BUFFER_ELEM_SIZE * SIGNAL_NUM;
	alignas(SignalBuffer<int64_t>) static int8_t SIGNAL_GLOAB_BUFFER[2 * SIGNAL_GLOAB_BUFFER_SIZE];


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

	template<typename A>
	class StackSignalTypes
	{
	public:
		StackSignalTypes(SignalThreadID tid, SignalID sid, const A & value)
		{
			SIGNAL_TYPE[sid] = type_name<A>::value;
			SIGNAL_THREAD[sid] = tid;
			
			new (SIGNAL_GLOAB_BUFFER + SIGNAL_GLOAB_BUFFER_SIZE * 0 + sid * SIGNAL_GLOAB_BUFFER_ELEM_SIZE) SignalBuffer<A>();
			new (SIGNAL_GLOAB_BUFFER + SIGNAL_GLOAB_BUFFER_SIZE * 1 + sid * SIGNAL_GLOAB_BUFFER_ELEM_SIZE) SignalBuffer<A>();
		}
	};

	auto offset(SignalThreadID tid, SignalID sid)
	{
		return SIGNAL_GLOAB_BUFFER_SIZE * supp_occupied_buf_id[tid] + sid * SIGNAL_GLOAB_BUFFER_ELEM_SIZE;
	}

	template<typename T>
	void push_back(SignalThreadID tid, SignalID sid, const T &x)
	{
		auto offset_ = SIGNAL_GLOAB_BUFFER_SIZE * supp_occupied_buf_id[tid] + sid * SIGNAL_GLOAB_BUFFER_ELEM_SIZE;
		// printf("push_back %d \n", offset_);
		reinterpret_cast<SignalBuffer<T>*>(SIGNAL_GLOAB_BUFFER + offset_) -> push_back(x);
		// auto tmp = reinterpret_cast<SignalBuffer<T>*>(SIGNAL_GLOAB_BUFFER + offset_);
		// printf("size %d \n", tmp->size());
		
	}

	template<typename T>
	auto get_buffer(SignalThreadID tid, SignalID sid)
	{
		auto offset_ = SIGNAL_GLOAB_BUFFER_SIZE * static_cast<BufferID>(1 - cons_instructed_buf_id[tid]) + sid * SIGNAL_GLOAB_BUFFER_ELEM_SIZE;
		return reinterpret_cast<SignalBuffer<T>*>(SIGNAL_GLOAB_BUFFER + offset_);
	}
	template<typename T>
	void reset_buffer(SignalThreadID tid, SignalID sid)
	{
		auto offset_ = SIGNAL_GLOAB_BUFFER_SIZE * static_cast<BufferID>(1 - cons_instructed_buf_id[tid]) + sid * SIGNAL_GLOAB_BUFFER_ELEM_SIZE;
		new (SIGNAL_GLOAB_BUFFER + offset_) SignalBuffer<T>();

	}
	// add unknown signal type to treat yet uninitialized signal

	// This must be called once for all threads which access before this class!!
	void supplier_occupy_buf(SignalThreadID tid) {
		//std::unique_lock<std::mutex> sync_lock(m_sync[tid]);  
		m_sync[tid].lock();
	}

	//multiple mutex lock type: wait type or passthrough type
std::condition_variable cv;

	void set67867(SignalThreadID tid)
	{
		//std::unique_lock<std::mutex> sync_lock(m_sync[tid]);  
		if(!data_supplied[tid])
		{
			data_supplied[tid] = true;
			supp_occupied_buf_id[tid] = cons_instructed_buf_id[tid];			
		}
		//cv.notify_one();
		
		m_sync[tid].unlock();
	}

	void set7897(SignalThreadID tid)
	{
		BufferID cibid = supp_occupied_buf_id[tid];

		std::unique_lock<std::mutex> sync_lock(m_sync[tid]);  
		if(!data_supplied[tid])
		{
			data_supplied[tid] = true;
			//supp_occupied_buf_id [tid] = static_cast<BufferID>(1 - cibid);	
			supp_occupied_buf_id[tid] = cons_instructed_buf_id[tid];			
			cv.notify_one();	
		}

	}

	void set(SignalThreadID tid)
	{
		std::unique_lock<std::mutex> sync_lock(m_sync[tid]);  
		if(!data_supplied[tid])
		{
			data_supplied[tid] = true;
			if(need_swap[tid]) {
				
				accept_swap[tid] = true;
				supp_occupied_buf_id[tid] = cons_instructed_buf_id[tid];	
			}		
			//cv.notify_one();	
		}
		printf("set: %d, %d, %d, %d\n", data_supplied[tid], need_swap[tid], accept_swap[tid], supp_occupied_buf_id[tid]);
	}
/*
	셋 : 이 버퍼 스왑을 수락  실제 버퍼 스왑과 데이터가 변함    수락 전에는 그 데이터 접근 금지

	확인 : 데이터 접근 전에 스왑 수락을 확인 후 되돌림 미수락시 스왑을 패스(공급 지연, 공급을 대기 x)
	스왑 : 스왑요청 작성   이다음 셋이 어떤 버퍼를 점유할지 지정
*/
	int16_t check()
	{
		int16_t result = 0;
		for(int tid = 0; tid < num_thread; ++tid)
		{
			std::unique_lock<std::mutex> sync_lock(m_sync[tid]);  
			if(need_swap[tid] && accept_swap[tid])
			{
				need_swap[tid] = false;		
				accept_swap[tid] = false;	
				result |= 1 << tid;
			}
			printf("check: %d, %d, %d, %d\n", data_supplied[tid], need_swap[tid], accept_swap[tid], static_cast<BufferID>(1 - cons_instructed_buf_id[tid]));
		}
		
		return result;
	}

	void swap()
	{
		for(int tid = 0; tid < num_thread; ++tid)
		{
			BufferID cibid = cons_instructed_buf_id[tid];

			std::unique_lock<std::mutex> sync_lock(m_sync[tid]);

			if(!need_swap[tid] && !accept_swap[tid])
			{
				data_supplied[tid] = false;
				need_swap[tid] = true;
				cons_instructed_buf_id [tid] = static_cast<BufferID>(1 - cibid);				
			}
			printf("swap: %d, %d, %d, %d\n", data_supplied[tid], need_swap[tid], accept_swap[tid], static_cast<BufferID>(1 - cons_instructed_buf_id[tid]));
		}
	}

	int16_t get()
	{
		int16_t result = 0;
		for(int tid = 0; tid < num_thread; ++tid)
		{
			bool new_data = false;
			BufferID cibid = cons_instructed_buf_id[tid];
			{
				std::unique_lock<std::mutex> sync_lock(m_sync[tid]);
				// cv.wait(sync_lock, []{ return ;});
				//cv.notify_one();
				new_data = data_supplied[tid];

				if(new_data)
				{
					data_supplied[tid] = false;
					// next set() will use this buffer
					cons_instructed_buf_id[tid] = supp_occupied_buf_id [tid];
				}
			}
			
			result |= new_data ? 1 << tid : 0;
		}

		return result;
	}

	// used to check if all thread fienushes their job at least one cycle
	int16_t get_read()
	{
		int16_t result = 0;
		for(int tid = 0; tid < num_thread; ++tid)
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

				// for(auto sid : THREAD_SIGNAL_ID[tid])
				// 	memcpy(cibid);



private:
	enum BufferID {
		m_buffer0 = 0,
		m_buffer1 = 1,
		m_one = 1
	};

	static constexpr const int num_thread = 1;
	std::array<std::mutex, num_thread> m_sync;

	// local
	// current buffer id which occupied by supplier
	std::array<BufferID, num_thread>  supp_occupied_buf_id{m_buffer0};

	//shared
	std::array<BufferID, num_thread>  cons_instructed_buf_id{m_buffer0};
	std::array<bool, num_thread>  data_supplied{true};

	std::array<bool, num_thread>  need_swap{false};
	std::array<bool, num_thread>  accept_swap{false};

	std::array<std::array<SignalID, SIGNAL_NUM>, num_thread>  THREAD_SIGNAL_ID;

};



SignalSync SSYNC;

#define SIGNAL_INIT_LOG_(TID) \
	SIGNAL_THREAD_ID = TID;

#define SIGNAL_INIT_LOG() \
	SSYNC.supplier_occupy_buf(SIGNAL_THREAD_ID);


#define SIGNAL_LOG(ID, VALUE) \
	static SignalSync::StackSignalTypes _##ID {SIGNAL_THREAD_ID, SignalID::ID, VALUE}; \
	SSYNC.push_back(SIGNAL_THREAD_ID, SignalID::ID, VALUE); 

#define SIGNAL_SYNC_LOG() \
	SSYNC.set(SIGNAL_THREAD_ID); 









void pack(flexbuffers::Builder &builder, SignalThreadID tid, SignalID sid, SignalType stype)
{

#define FLEXBUFFER(TYPE, type) \
	builder.Vector([&]() \
	{ \
		auto buf = SSYNC.get_buffer<type>(tid, sid); \
		for (int i = 0; i < buf->size(); i++) {\
			const auto a = (*buf).pop_front(); \
			printf("%d, ", a); \
			builder.TYPE(a); 	\
		} \
		printf("\n"); \
	});


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
			first |= SSYNC.get_read();
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
		rc = mosquitto_connect(mosq, "192.168.0.29", 9884, 60);
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


		auto dataRate = std::chrono::milliseconds(1);
		// std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

		while (true)
		{
			auto start = std::chrono::high_resolution_clock::now();

			if(conn.is_ready())
				publish_sensor_data(mosq);


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

		auto result = SSYNC.check();


		int32_t out_flag = conn.get_type();

		flexbuffers::Builder builder;
		builder.Vector([&]() {
			builder.Int(out_flag);
			
			for(int sid = 0; sid < SIGNAL_NUM; ++sid)
			{
				auto tid = SIGNAL_THREAD[sid];
				if (result & 1 << tid && out_flag & 1 << sid)
				{
					
					pack(builder, tid, static_cast<SignalID>(sid), SIGNAL_TYPE[sid]);
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


		SSYNC.swap();

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
				int32_t sid = flexbuffers::GetRoot(received_buffer).AsInt32();

				printf("requested signal list : %d : ", sid);
				for (size_t j = 0; j < SIGNAL_NUM; j++) {
					printf("%d", (bool)(sid & 1 << j));
				}
				printf("\n");

				
				connn.set_type(sid);
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



		void set_type(int32_t rec_type)
		{
			mt.set(rec_type);
		}

		int32_t get_type()
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

				

					for(int sid = 0; sid < SIGNAL_NUM; ++sid)
					{
						std::cout << SIGNAL_NAME[sid] << ", ";
					}
					std::cout << std::endl;


			builder.Vector([&]() {
				builder.Vector([&]()
				{
					for(int sid = 0; sid < SIGNAL_NUM; ++sid)
					{
						builder.String(SIGNAL_NAME[sid]);
					}
				});
				// builder.Vector(SIGNAL_NAME, (size_t)SIGNAL_NUM);
				/*
					!!! THIS MAY BE UNSAFE READ !!!
				*/
				builder.Vector([&]()
				{
					for(int sid = 0; sid < SIGNAL_NUM; ++sid)
					{
						builder.Int(SIGNAL_TYPE[sid]);
					}
				});

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
			void set(int32_t rec_type)
			{
				std::lock_guard<std::mutex> mm(m);
				signal_type = rec_type;
				receiv = true;
			}
			int32_t get()
			{
				std::lock_guard<std::mutex> mm(m);
				return signal_type;
			}
		private:

			std::mutex m;
			int32_t signal_type{0};
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
	SIGNAL_INIT_LOG_(s_no_name_thread);
	int buffer[16]{0};

	while (true)
	{
		
	//	SIGNAL_INIT_LOG();

		
		SIGNAL_LOG(wefn, 1);
		buffer[0] = (buffer[0]+1) % 120;
		printf("%d ", buffer[0]);
		SIGNAL_LOG(dvweb, buffer[0]);
		SIGNAL_LOG(greht, 2.0f);

		SIGNAL_LOG(fewfg, -120);
		
		SIGNAL_LOG(fsdfsf, 543);
	std::this_thread::sleep_for(std::chrono::microseconds(100));
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
