#include <iostream>
#include <chrono>
#include <thread>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>


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


enum SignalType : int16_t
{
	t_yet = 0, // net specified yet
	t_bool,
	t_float,
	t_double,
	t_int
};


template <typename F1, typename F2>
class ClientViewer
{
public:	
	ClientViewer(std::string client_id_, std::string device_id_, F1 &nametype_callback, F2 &signal_callback) :
		conn(client_id_, device_id_, nametype_callback, signal_callback)
	{
			
	}

	~ClientViewer()
	{
		mosquitto_loop_stop(mosq, false);
   		mosquitto_destroy(mosq);
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
					// if device disconnected here ? 
				}
				case Connection::Announce::ann_reply:
				{
					printf("device announce : ann_reply\n");
					connn.cp_advance_wl();
					break;
				}

				/*
					next state should be 
						receive signal list
						or
						Connection::Announce  (because of device side reason)

					other sate should be ignored
				*/

				// case device off: ?
			}
		}
		else if (strcmp(msg->topic, (connn.device_id + "/slist").c_str()) == 0)
		{
			printf("device sent signal list.\n");
			if (connn.cp_advance_done()) {
				auto root = flexbuffers::GetRoot(received_buffer).AsVector();
				auto sname = root[0].AsVector();
				auto stype = root[1].AsVector();

				connn.mt.set(sname, stype);

				connn.nametype_callback(connn.mt.get_name(), connn.mt.get_type());

				connn.publish_slist(mosq);
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



	// template <typename F>
	class Connection
	{
	public:
		Connection(std::string client_id_, std::string device_id_, F1 &nametype_callback_, F2 &callback) :
			client_id(client_id_),  device_id(device_id_), nametype_callback(nametype_callback_), out_callback{callback}
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

		void publish_slist(struct mosquitto *mosq)
		{
			flexbuffers::Builder builder;
			builder.Int(mt.get());
			builder.Finish();
			auto K = builder.GetBuffer();

			int rc = mosquitto_publish(mosq, NULL, (device_id + "/cslist").c_str(), K.size(), K.data(), 2, false);
			if (rc != MOSQ_ERR_SUCCESS)
			{
				fprintf(stderr, "Error publishing: %s\n", mosquitto_strerror(rc));
			}
		}

		class MessageType 
		{
		public:
			void set(flexbuffers::Vector &x, flexbuffers::Vector &y)
			{
				std::lock_guard<std::mutex> mm(m);
				sname.resize(0);
				stype.resize(0);

				for (size_t j = 0; j < x.size(); j++) {
					sname.push_back(x[j].AsString().str());
					std::cout << x[j].AsString().str();
				}
				std::cout << std::endl;
				

				for (size_t j = 0; j < y.size(); j++) {
					stype.push_back(y[j].AsInt32());
					std::cout << y[j].AsInt32();
				}
				std::cout << std::endl;

				receiv = true;
			}
			auto get_name()
			{
				std::lock_guard<std::mutex> mm(m);
				return sname;
			}
			auto get_type()
			{
				std::lock_guard<std::mutex> mm(m);
				return stype;
			}

			bool is_ready()
			{
				std::lock_guard<std::mutex> mm(m);
				return receiv;
			}

			int32_t get()
			{
				std::lock_guard<std::mutex> mm(m);
				return signal_type;
			}



			void set_ids(const std::vector<int> &sid)
			{
				std::lock_guard<std::mutex> mm(m);
				int32_t cont = 0;
				for (auto id : sid)
				{
					cont |= 1 << id;
				}
				signal_type = cont;
			}
		private:

			std::mutex m;
			int32_t signal_type{1<<1};

			std::vector<std::string> sname;
			std::vector<int> stype;


			bool receiv {false};
		};

		MessageType mt;
		F1 nametype_callback;
		F2 out_callback;
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