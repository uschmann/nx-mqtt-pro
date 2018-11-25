#include <string.h>
#include <stdio.h>

#include <switch.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include "mqtt.h"



void init() {
	gfxInitDefault();
	consoleInit(NULL);
	socketInitializeDefault();
	#ifdef DEBUG
  	nxlinkStdio();
  	printf("printf output now goes to nxlink server\n");
  	#endif // DEBUG
}

void publish_callback(void** unused, struct mqtt_response_publish *published) 
{
    /* not used in this example */
}

int open_nb_socket(const char* addr, const char* port) {
    struct addrinfo hints = {0};

    hints.ai_family = AF_UNSPEC; /* IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Must be TCP */
    int sockfd = -1;
    int rv;
    struct addrinfo *p, *servinfo;

    /* get address information */
    rv = getaddrinfo(addr, port, &hints, &servinfo);
    if(rv != 0) {
        fprintf(stderr, "Failed to open socket (getaddrinfo): %s\n", gai_strerror(rv));
        return -1;
    }

    /* open the first possible socket */
    for(p = servinfo; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) continue;

        /* connect to server */
        rv = connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen);
        if(rv == -1) continue;
        break;
    }  

    /* free servinfo */
    freeaddrinfo(servinfo);

    /* make non-blocking */
    if (sockfd != -1) fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL) | O_NONBLOCK);

    /* return the new socket fd */
    return sockfd;  
}


int main(int argc, char **argv)
{
	init();

	const char* addr = "test.mosquitto.org";
    const char* port = "1883";

	int sockfd = open_nb_socket(addr, port);
	if (sockfd == -1) {
        printf("Error conecting to socket");
    }
	else {
		printf("Conected to broker\n");
		/* setup a client */
		struct mqtt_client client;
		uint8_t sendbuf[2048]; /* sendbuf should be large enough to hold multiple whole mqtt messages */
		uint8_t recvbuf[1024]; /* recvbuf should be large enough any whole mqtt message expected to be received */
		mqtt_init(&client, sockfd, sendbuf, sizeof(sendbuf), recvbuf, sizeof(recvbuf), publish_callback);
		mqtt_connect(&client, "publishing_client", NULL, NULL, 0, NULL, NULL, 0, 400);

		int x = 0;

		// Main loop
		while(appletMainLoop())
		{
			//Scan all the inputs. This should be done once for each frame
			hidScanInput();

			// Your code goes here
			//hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
			u32 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

			if (kDown & KEY_PLUS) break; // break in order to return to hbmenu

			if(kDown & KEY_A) {
				const char* topic = "/usch/nx/foo";
				char application_message[256] = "";
				x++;
				sprintf(application_message, "Message from switch: %d", x);
				printf("topic: %s\n", topic);
				mqtt_publish(&client, topic, application_message, strlen(application_message) + 1, MQTT_PUBLISH_QOS_0);
				
				if (client.error != MQTT_OK) {
					printf("error: %s\n", mqtt_error_str(client.error));
				}
			}

			mqtt_sync((struct mqtt_client*) &client);

			gfxFlushBuffers();
			gfxSwapBuffers();
			gfxWaitForVsync();
		}
	}

    socketExit();

	gfxExit();
	return 0;
}

