#include <XnOpenNI.h>
#include <XnCppWrapper.h>
#include "control.h"

#define CONFIG_PATH "data/config.xml"

#define CHECK_RC(rc, what) 								\
	if(rc != XN_STATUS_OK) { 							\
		fprintf(stderr, "%s failed: %s\n", what, xnGetStatusString(rc));	\
		return rc;								\
	}


xn::Context g_Context;
xn::ScriptNode g_ScriptNode;
xn::DepthGenerator g_DepthGenerator;
xn::GestureGenerator g_GestureGenerator;
xn::HandsGenerator g_HandsGenerator;

int main(int argc, char *argv[]) {
	XnStatus rc;
	xn::EnumerationErrors errors;
	XnCallbackHandle cbhandle;

	//Connect to fakenav server.
	init_control(argv[0], atoi(argv[1]));
	conf();

	//Create context from settings file.
	rc = g_Context.InitFromXmlFile(CONFIG_PATH, g_ScriptNode, &errors);
	if(rc == XN_STATUS_NO_NODE_PRESENT) {
		XnChar strError[1024];
		errors.ToString(strError, 1024);
		fprintf(stderr, "%s\n", strError);
		return rc;
	} else if(rc != XN_STATUS_OK) {
		fprintf(stderr, "Open failed: %s\n", xnGetStatusString(rc));
		return rc;
	}

	//Create generators.
	rc = g_Context.FindExistingNode(XN_NODE_TYPE_DEPTH, g_DepthGenerator);
	CHECK_RC(rc, "Find DepthGenerator");

	rc = g_GestureGenerator.Create(g_Context);
	CHECK_RC(rc, "Create GestureGenerator");

	rc = g_HandsGenerator.Create(g_Context);
	CHECK_RC(rc, "Create HandsGenerator");

	//Register callabcks.
	rc = g_GestureGenerator.RegisterGestureCallbacks(Gesture_Recognized, Gesture_Process, NULL, cbhandle);
	CHECK_RC(rc, "Register gesture callabcks");

	rc = g_HandsGenerator.RegisterHandCallbacks(Hand_Create, Hand_Update, Hand_Destroy, NULL, cbhandle);
	CHECK_RC(rc, "Register hand callbacks");

	//Add gestures.
	rc = g_Context.StartGeneratingAll();
	CHECK_RC(rc, "Generating");

	rc = g_GestureGenerator.AddGesture("Wave", NULL);
	CHECK_RC(rc, "Add wave gesture");

	while(1) {
		g_Context.WaitOneUpdateAll(g_DepthGenerator);
		update_control();
	}
}
