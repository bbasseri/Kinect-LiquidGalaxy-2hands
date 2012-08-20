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
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <libconfig.h++>


#include "control.h"


#define round(x) ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))

using namespace std;
using namespace libconfig;

char buf[256];
int _server = -1;
int numHands = 0;
bool autoRun;
string autoRunSource;

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

//Configurable values:
float thresh;
float smooth;

float scaleX;
float scaleY;
float scaleZ;
float scaleYaw;
float scalePitch;

float zLimit;

//HandsGenerator for tracking hands.
extern xn::HandsGenerator g_HandsGenerator;


void init_control(const char *serveraddr, int portno) {

	_server = connect_to_server(serveraddr, portno);
	if(_server == -1) {
		fprintf(stderr, "Error connecting to fakenav\n");
		exit(1);
	}
	// conf();
}

/* Update fakenave with latest controls.
 * deltaX should pan the camera left/right.
 * deltaY should move you closer/farther to the center of the earth.
 * deltaZ should control the speed with which you move forward.
 */
void update_control() {

	// Setting 0 to axis based on hands being tracked or lost
	if(trackingMove == false){
		x = 0.0f;
		y = 0.0f;
		z = 0.0f;
	}
	if(trackingLook == false){
		yaw = 0.0f;
		pitch = 0.0f;
	}

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
		numHands++;
	} else if(!trackingLook) {
		trackingLook = true;
		startLook = *position;
		lookId = handId;
		numHands++;
	}
}

/* When the hand moves, update the controls sent to fakenav. */
void XN_CALLBACK_TYPE Hand_Update(xn::HandsGenerator &generator,
                                  XnUserID handId,
                                  const XnPoint3D *position,
                                  XnFloat time,
                                  void *cookie) {

    if(moveId == handId) {
		XnFloat deltaX = startMove.X - position->X;
		XnFloat deltaY = startMove.Y - position->Y;
		XnFloat deltaZ = startMove.Z - position->Z;

		if(deltaZ - deltaY > zLimit){
			x = threshCalulate(deltaX, -scaleX);
			y = threshCalulate(deltaZ, -scaleZ);
			z = threshCalulate(deltaY, scaleY);
		}else{
			x=0;
			y=0;
		    z=0;
		    trackingMove = false;
		    generator.xn::HandsGenerator::StopTracking(moveId);
		}

    } else if(lookId == handId) {
		XnFloat deltaX = startLook.X - position->X;
		XnFloat deltaY = startLook.Y - position->Y;
		XnFloat deltaZ = startLook.Z - position->Z;

		if(deltaZ - deltaY > zLimit){
			yaw = threshCalulate(deltaX, -scaleYaw);
			pitch = threshCalulate(deltaY, -scalePitch);
		}else{
			yaw=0;
			pitch=0;
		    trackingLook = false;
		    generator.xn::HandsGenerator::StopTracking(lookId);
		}
	}
}

/* Calculates x y z, based on thresh, smooth, and scale */
float threshCalulate(XnFloat delta, float scale){
	if(delta < thresh && delta > -thresh){
		return 0.0f;
	} else if (delta < 0.0f) {
		return (round(delta) / smooth) * scale;
	} else{
		return (round(delta) / smooth) * scale;
	}
}


/* Hand is gone, should stop updating fakenav... */
void XN_CALLBACK_TYPE Hand_Destroy(xn::HandsGenerator &generator,
                                   XnUserID handId,
                                   XnFloat time,
                                   void *cookie) {


    printf("Lost hand: %d\n", handId);

    // When both hands lost if specified in config file, run binary
    // script that is provided (Config file functionality) (TODO)

	if(moveId == handId) {
		trackingMove = false;
	} else if(lookId == handId) {
	    trackingLook = false;
	}
	numHands--;
	if (numHands == 0){
		if (autoRun == 1 && autoRunSource != ""){
			// int pid_ps = fork();
    		// if (pid_ps == 0) {
				printf(".%s. \n",autoRunSource.c_str());
        		if (system(autoRunSource.c_str()))
        			printf("Script executed");
    		// }
		}
	}

}

int conf()
{
		Config cfg;
		// Read the file. If there is an error, report it and exit.
		cfg.readFile("./config.ini");

		scaleX = cfg.lookup("scaleX");
		scaleY = cfg.lookup("scaleY");
		scaleZ = cfg.lookup("scaleZ");

		scaleYaw = cfg.lookup("scaleYaw");
		scalePitch = cfg.lookup("scalePitch");

		zLimit = cfg.lookup("zLimit");

		thresh = cfg.lookup("thresh");
		smooth = cfg.lookup("smooth");

		autoRun = cfg.lookup("autoRun");

		if(cfg.lookupValue("autoRunSource", autoRunSource))
		{

		}

		cout << "X: " << scaleX << endl << endl;
		cout << "Y: " << scaleY << endl << endl;
		cout << "Z: " << scaleZ << endl << endl;
		cout << "yaw: " << scaleYaw << endl << endl;
		cout << "pitch: " << scalePitch << endl << endl;
		cout << "zLimit: " << zLimit << endl << endl;
		cout << "thresh: " << thresh << endl << endl;
		cout << "smooth: " << smooth << endl << endl;
		cout << "autoRun: " << autoRun << endl << endl;
		cout << "autoRunSource: ." << autoRunSource << "." << endl << endl;
}
