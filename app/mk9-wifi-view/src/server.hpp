#include <iostream>
#include <chrono>
#include <thread>
#include <cstring>


#include <functional>


#include "mosquitto.h"

#include "efp.hpp"


#include "plotlib.hpp"
/*
 * This example shows how to write a client that subscribes to a topic and does
 * not do anything other than handle the messages that are received.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#include "flatbuffers/flexbuffers.h"




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

template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}


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






// typedef void (*signal_callback_t)(const flexbuffers::Vector &);

// typedef void (*NametypeCallbackType_t)(const std::vector<ThreadSignal>&);

template<
	typename F1 = std::function<void(const flexbuffers::Vector &)>,
	typename F2 = std::function<void(const flexbuffers::Vector &)>,
	typename F3 = std::function<void(const bool &)>>
class ClientViewer
{
public:	
	

	//template<typename F1,typename F2>
	ClientViewer(std::string client_id_, std::string device_id_, F1 nametype_callback, F2 signal_callback, F3 disconnected_callback) :
		conn(client_id_, device_id_, nametype_callback, signal_callback, disconnected_callback)
	{
			
	}

	~ClientViewer()
	{
		mosquitto_loop_stop(mosq, false);
   		mosquitto_destroy(mosq);

		// todo: this command will close all session;
		mosquitto_lib_cleanup();
	}
	


	
	void init() 
	{
		int rc;

		mosquitto_lib_init();

		mosq = mosquitto_new(conn.client_id.c_str(), true, (void *)&conn);
		if(mosq == NULL){
			fprintf(stderr, "Error: Out of memory.\n");
			abort();
		}

		mosquitto_connect_callback_set(mosq, on_connect);
		mosquitto_subscribe_callback_set(mosq, on_subscribe);
		mosquitto_message_callback_set(mosq, on_message);
		{
			flexbuffers::Builder builder;
			builder.Int(Connection::Announce::ann_off);
			builder.Finish();
			auto K = builder.GetBuffer();
			mosquitto_will_set(mosq, (conn.device_id + "/ca").c_str(), K.size(), K.data(), 2, false); 
		}

		rc = mosquitto_connect(mosq, "192.168.0.29", 9884, 60);
		if(rc != MOSQ_ERR_SUCCESS){
			mosquitto_destroy(mosq);
			fprintf(stderr, "Error: %s\n", mosquitto_strerror(rc));
			abort();
		}

		rc = mosquitto_loop_start(mosq);
		if (rc != MOSQ_ERR_SUCCESS)
		{
			mosquitto_destroy(mosq);
			fprintf(stderr, "Error: %s\n", mosquitto_strerror(rc));
		}
	}

void publish_slist(const std::vector<std::vector<SignalID>> &slist)
	{
		conn.publish_slist(mosq, slist);
	}


private:


	/* Callback called when the client receives a CONNACK message from the broker. */
	static void on_connect(struct mosquitto *mosq, void *obj, int reason_code)
	{
		Connection &connn = *(Connection *)obj;
		int rc;
		/* Print out the connection result. mosquitto_connack_string() produces an
		* appropriate string for MQTT v3.x clients, the equivalent for MQTT v5.0
		* clients is mosquitto_reason_string().
		*/
		printf("on_connect: %s\n", mosquitto_connack_string(reason_code));
		if(reason_code != 0){
			/* If the connection fails for any reason, we don't want to keep on
			* retrying in this example, so disconnect. Without this, the client
			* will attempt to reconnect. */
			mosquitto_disconnect(mosq);
		}

		/* Making subscriptions in the on_connect() callback means that if the
		* connection drops and is automatically resumed by the client, then the
		* subscriptions will be recreated when the client reconnects. */
		rc = mosquitto_subscribe(mosq, NULL, connn.device_id.c_str(), 0);
		if(rc != MOSQ_ERR_SUCCESS){
			fprintf(stderr, "Error subscribing: %s\n", mosquitto_strerror(rc));
			mosquitto_disconnect(mosq);
		}

		rc = mosquitto_subscribe(mosq, NULL, (connn.device_id + "/da").c_str(), 2);
		if(rc != MOSQ_ERR_SUCCESS){
			fprintf(stderr, "Error subscribing: %s\n", mosquitto_strerror(rc));
			mosquitto_disconnect(mosq);
		}

		rc = mosquitto_subscribe(mosq, NULL, (connn.device_id + "/slist").c_str(), 2);
		if(rc != MOSQ_ERR_SUCCESS){
			fprintf(stderr, "Error subscribing: %s\n", mosquitto_strerror(rc));
			mosquitto_disconnect(mosq);
		}

		connn.publish_announce(mosq, Connection::Announce::ann_on);
	}


	/* Callback called when the broker sends a SUBACK in response to a SUBSCRIBE. */
	static void on_subscribe(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos)
	{
		int i;
		bool have_subscription = false;

		/* In this example we only subscribe to a single topic at once, but a
		* SUBSCRIBE can contain many topics at once, so this is one way to check
		* them all. */
		for(i=0; i<qos_count; i++){
			printf("on_subscribe: %d:granted qos = %d\n", i, granted_qos[i]);
			if(granted_qos[i] <= 2){
				have_subscription = true;
			}
		}
		if(have_subscription == false){
			/* The broker rejected all of our subscriptions, we know we only sent
			* the one SUBSCRIBE, so there is no point remaining connected. */
			fprintf(stderr, "Error: All subscriptions rejected.\n");
			mosquitto_disconnect(mosq);
		}
	}


	/* Callback called when the client receives a message. */
	static void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
	{
		std::vector<uint8_t> received_buffer(static_cast<uint8_t*>(msg->payload), static_cast<uint8_t*>(msg->payload) + msg->payloadlen);
		Connection &connn = *(Connection *)obj;

	
		if (strcmp(msg->topic, (connn.device_id + "/da").c_str()) == 0)
		{
			auto ann = static_cast<typename Connection::Announce>(flexbuffers::GetRoot(received_buffer).AsInt32());
			
			switch(ann)
			{
				case Connection::Announce::ann_on:
				{
					printf("device announce : ann_on\n");
					// reset all data!!!
					//
					connn.publish_announce(mosq, Connection::Announce::ann_reply);
					connn.cp_advance_wl();
					break;
				}
				case Connection::Announce::ann_reply:
				{
					printf("device announce : ann_reply\n");
					connn.cp_advance_wl();
					break;
				}
				case Connection::Announce::ann_off:
				{
					printf("device announce : ann_off\n");
					connn.cp_reset();
					connn.disconnected_callback(true);
					break;
				}
				/*
					next state should be 
						receive signal list
						or
						Connection::Announce  (because of device side reason)

					other sate should be ignored
				*/
			}
		}
		else if (strcmp(msg->topic, (connn.device_id + "/slist").c_str()) == 0)
		{
			printf("device sent signal list.\n");
			if (connn.cp_advance_done()) {
				auto root = flexbuffers::GetRoot(received_buffer).AsVector();

				// connn.mt.set(root);

				connn.nametype_callback(root);//connn.mt.get_list());
				
				// todo: not be here
				// connn.publish_slist(mosq);
				// dont init device from here. just set device init state as "sned noting"
			}
		}
		else if (strcmp(msg->topic, connn.device_id.c_str()) == 0)
		{
			// printf("device: signal.\n");
			if (connn.cp_is_done()) {			
				auto root = flexbuffers::GetRoot(received_buffer).AsVector();
				
				connn.out_callback(root);
			}
		}

	}



	class Connection
	{
	public:
		
		Connection(std::string client_id_, std::string device_id_, F1 &nametype_callback_, F2 &callback, F3 &disconnected_callback) :
			client_id(client_id_),  device_id(device_id_), nametype_callback(nametype_callback_), out_callback{callback}, disconnected_callback{disconnected_callback}
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
			cp_wait_list,
			cp_done,
			done
		};
		enum ConnState {
			connected,
			disconnected,
		};
		void cp_reset()
		{
			cp = cp_wait_response;
			// other reset work.
		}
		bool cp_advance_wl()
		{
			if(cp == cp_wait_response)
			{
				cp = cp_wait_list;
				return true;
			}
			return false;
		}
		bool cp_advance_done()
		{
			if(cp == cp_wait_list || cp_done)
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

		void publish_announce(struct mosquitto *mosq, Announce ann)
		{
			flexbuffers::Builder builder;
			builder.Int(ann);
			builder.Finish();
			auto K = builder.GetBuffer();

			int rc = mosquitto_publish(mosq, NULL, (device_id + "/ca").c_str(), K.size(), K.data(), 2, false);
			if (rc != MOSQ_ERR_SUCCESS)
			{
				fprintf(stderr, "Error publishing: %s\n", mosquitto_strerror(rc));
			}
		}

		// todo: fix: when new device subscribed, send rq list
		void publish_slist(struct mosquitto *mosq, const std::vector<std::vector<SignalID>> &rquested)
		{

			flexbuffers::Builder builder;

			builder.Vector([&]() {

				for (auto &signal_list : rquested)
				{
					builder.Vector([&]() {
						for(auto sid : signal_list)
						{
							builder.Int(sid);
						}
					});
				}
			});

			builder.Finish();
			auto K = builder.GetBuffer();

			int rc = mosquitto_publish(mosq, NULL, (device_id + "/cslist").c_str(), K.size(), K.data(), 2, false);
			if (rc != MOSQ_ERR_SUCCESS)
			{
				fprintf(stderr, "Error publishing: %s\n", mosquitto_strerror(rc));
			}
		}

		F1 nametype_callback;
		F2 out_callback;
		F3 disconnected_callback;

		std::string client_id;
		std::string device_id;
		// mosquitto_subscribe topic

	private:
		ConnPhase cp{cp_wait_response};
		ConnState cs{disconnected};
		

	};

	
	Connection conn;
	struct mosquitto *mosq;
};