void update_control();
void init_control(const char *serveraddr, int port);
int connect_to_server(const char *address, int portno);


void XN_CALLBACK_TYPE Gesture_Recognized(xn::GestureGenerator &generator,
                                         const XnChar *gesture,
                                         const XnPoint3D *startPosition,
                                         const XnPoint3D *endPosition,
                                         void *cookie);

void XN_CALLBACK_TYPE Gesture_Process(xn::GestureGenerator &generator,
                                      const XnChar *gesture,
                                      const XnPoint3D *position,
                                      XnFloat progress,
                                      void *cookie);

void XN_CALLBACK_TYPE Hand_Create(xn::HandsGenerator &generator,
                                  XnUserID handId,
                                  const XnPoint3D *position,
                                  XnFloat time,
                                  void *cookie);

void XN_CALLBACK_TYPE Hand_Update(xn::HandsGenerator &generator,
                                  XnUserID handId,
                                  const XnPoint3D *position,
                                  XnFloat time,
                                  void *cookie);

void XN_CALLBACK_TYPE Hand_Destroy(xn::HandsGenerator &generator,
                                   XnUserID handId,
                                   XnFloat time,
                                   void *cookie);
