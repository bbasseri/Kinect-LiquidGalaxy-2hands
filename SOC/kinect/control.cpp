#include <XnCppWrapper.h>
#include <XnOpenNI.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "control.h"


#define round(x) ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))

char buf[256];
int _server = -1;

//Are we tracking hands?
XnBool trackingMove = false;
XnBool trackingLook = false;

//Position of the hands when we started tracking.
XnPoint3D startMove;
XnPoint3D startLook;

XnUserID moveId;
XnUserID lookId;

XnFloat x = 0.0f,
	y = 0.0f,
	z = 0.0f,
	yaw = 0.0f,
	pitch = 0.0f,
	roll = 0.0f;

//HandsGenerator for tracking hands.
extern xn::HandsGenerator g_HandsGenerator;

//
int mode = 0;

void init_control(const char *serveraddr, int portno) {
	_server = connect_to_server(serveraddr, portno);

	if(_server == -1) {
		fprintf(stderr, "Error connecting to fakenav\n");
		exit(1);
	}
}

/* Update fakenave with latest controls.
 * deltaX should pan the camera left/right.
 * deltaY should move you closer/farther to the center of the earth.
 * deltaZ should control the speed with which you move forward.
 */
void update_control() {

	//x, y, z, yaw, pitch, roll
	sprintf(buf, "%f, %f, %f, %f, %f, %f\n",
		      x, y, z, yaw, pitch, 0.0f);

	int len = strlen(buf);

	if(_server > 0 && write(_server, buf, len) != len) {
		fprintf(stderr, "Error writing to fakenav\n");
		exit(1);
	}
}

int connect_to_server(const char *address, int portno) {
	int sockfd, n;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0) {
		fprintf(stderr, "Error opening socket.\n");
		exit(1);
	}

	server = gethostbyname(address);
	if(server == NULL) {
		fprintf(stderr, "ERROR: No such host.\n");
		exit(1);
	}

	bzero((char *)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(portno);
	if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) <0) {
		fprintf(stderr, "Error connecting.\n");
		exit(1);
	}

	return sockfd;
}


/* Start tracking the hand once a gesture is recognized */
void XN_CALLBACK_TYPE Gesture_Recognized(xn::GestureGenerator &generator,
                                         const XnChar *gesture,
                                         const XnPoint3D *startPosition,
                                         const XnPoint3D *endPosition,
                                         void *cookie) {
        printf("Gesture recognized: %s\n", gesture);

                // Only start tracking a hand if we're not already tracking one.
       if(!trackingMove || !trackingLook) {
		g_HandsGenerator.StartTracking(*endPosition);
	}
}

/* We don't care about progress of a gesture. */
void XN_CALLBACK_TYPE Gesture_Process(xn::GestureGenerator &generator,
                                      const XnChar *gesture,
                                      const XnPoint3D *position,
                                      XnFloat progress,
                                      void *cookie) {
}

/* Record the position of a new hand. This will be the reference point for ... */
void XN_CALLBACK_TYPE Hand_Create(xn::HandsGenerator &generator,
                                  XnUserID handId,
                                  const XnPoint3D *position,
                                  XnFloat time,
                                  void *cookie) {
        printf("New Hand: %d @(%f, %f, %f)\n", handId, position->X, position->Y, position->Y
);
	if(!trackingMove) {
	        trackingMove = true;
	        startMove = *position;
		moveId = handId;
	} else if(!trackingLook) {
		trackingLook = true;
		startLook = *position;
		lookId = handId;
	}
}


/* When the hand moves, update the controls sent to fakenav. */
void XN_CALLBACK_TYPE Hand_Update(xn::HandsGenerator &generator,
                                  XnUserID handId,
                                  const XnPoint3D *position,
                                  XnFloat time,
                                  void *cookie) {
	float scale = 4.0f;
	float smooth = 5.0f;
	float thresh = 20.0f;


    if(moveId == handId) {
		XnFloat deltaX = startMove.X - position->X;
		XnFloat deltaY = startMove.Y - position->Y;
		XnFloat deltaZ = startMove.Z - position->Z;

		if(deltaX < thresh && deltaX > -thresh){
			x = 0.0f;
		} else if (deltaX < 0.0f) {
			x = (round(deltaX) / smooth) * -scale;
		} else{
			x = (round(deltaX) / smooth) * -scale;
		}


		if(deltaY < thresh && deltaY > -thresh){
			y = 0.0f;
		} else if (deltaY < 0.0f) {
			y = (round(deltaZ) / smooth) * -scale;
		} else{
			y = (round(deltaZ) / smooth) * -scale;
		}

		if(deltaZ < thresh && deltaZ > -thresh){
			z = 0.0f;
		} else if (deltaZ < 0.0f) {
			z = (round(deltaY) / smooth) * scale;
		} else{
			z = (round(deltaY) / smooth) * scale;
		}

		// x = (round(deltaX) / smooth) * -scale;
		// y = (round(deltaZ) / smooth) * -scale;
		// z = (round(deltaY) / smooth) * scale;
    } else if(lookId == handId) {
		XnFloat deltaX = startLook.X - position->X;
		XnFloat deltaY = startLook.Y - position->Y;
		XnFloat deltaZ = startLook.Z - position->Z;

		if(deltaX < thresh && deltaX > -thresh){
			yaw = 0.0f;
		} else if (deltaX < 0.0f) {
			yaw = (round(deltaX) / smooth) * -scale;
		} else{
			yaw = (round(deltaX) / smooth) * -scale;
		}

		if(deltaY < thresh && deltaY > -thresh){
			pitch = 0.0f;
		} else if (deltaY < 0.0f) {
			pitch = (round(deltaY) / smooth) * -scale;
		} else{
			pitch = (round(deltaY) / smooth) * -scale;
		}

		// yaw = (round(deltaX) / smooth) * -scale;
		// pitch = (round(deltaY) / smooth) * -scale;
	}
}

/* Hand is gone, should stop updating fakenav... */
void XN_CALLBACK_TYPE Hand_Destroy(xn::HandsGenerator &generator,
                                   XnUserID handId,
                                   XnFloat time,
                                   void *cookie) {
        printf("Lost hand: %d\n", handId);
	if(moveId == handId) {
		trackingMove = false;
	} else if(lookId == handId) {
	        trackingLook = false;
	}
}

