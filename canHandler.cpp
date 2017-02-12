#include "canHandler.h"

canHandler::canHandler(log4cpp::Category *logger, int canId)
{
    //ctor
    this->logger = logger;
    this->canId = canId;
    this->tcpserver = nullptr;
    pthread_mutex_init(&m_mutex, NULL);
    pthread_cond_init(&m_condv, NULL);
    pthread_mutex_init(&m_mutex_in, NULL);
    pthread_cond_init(&m_condv_in, NULL);
    pb_pressed = false;
}

canHandler::~canHandler()
{
    //dtor
    pthread_mutex_destroy(&m_mutex);
    pthread_cond_destroy(&m_condv);
    pthread_mutex_destroy(&m_mutex_in);
    pthread_cond_destroy(&m_condv_in);
}
/**
 * @brief Return the canid being used
 * @return
 */
int canHandler::getCanId(){
    return canId;
}
/**
 * @brief Set the gpio pins for the push button, the green led and the yelow led
 * @param pbutton raspberry pi gpio number
 * @param gledpin raspberry pi gpio number
 * @param yledpin raspberry pi gpio number
 */
void canHandler::setPins(int pbutton,int gledpin,int yledpin){
    pbpin = pbutton;
    glpin = gledpin;
    ylpin = yledpin;
    string s;
    stringstream ss;

    ss<<pbpin;
    logger->debug("[canHandler] Setting PB to pin %s",ss.str().c_str());
    pb = gpio(ss.str());

    ss.clear();ss.str("");
    ss<<glpin;
    logger->debug("[canHandler] Setting Green Led to pin %s",ss.str().c_str());
    gl = gpio(ss.str());

    ss.clear();ss.str("");
    ss<<ylpin;
    logger->debug("[canHandler] Setting Yellow Led to pin %s",ss.str().c_str());
    yl = gpio(ss.str());

    pb.export_gpio();
    gl.export_gpio();
    yl.export_gpio();

    pb.setdir_gpio("in");
    gl.setdir_gpio("out");
    yl.setdir_gpio("out");
    if (config->getNodeMode() == 0){
        logger->info("[canHandler] Node is in SLIM mode");
    	gl.setval_gpio("1");
	yl.setval_gpio("0");
    }
    else{
        logger->info("[canHandler] Node is in FLIM mode");
    	gl.setval_gpio("0");
	yl.setval_gpio("1");
    }
}
/**
 * @brief Sets the configurator object to extracte the module properties
 * @param config
 */
void canHandler::setConfigurator(nodeConfigurator *config){
    this->config = config;
    //config->setNodeParams(MANU_MERG,MSOFT_MIN_VERSION,MID,0,0,config->getNumberOfNVs(),MSOFT_VERSION,MFLAGS);
 }
/**
 * @brief Put an existing tcp server in the queue to receive and send CAN frames
 * @param tcpserver
 */
void canHandler::setTcpServer(tcpServer * tcpserver){
    servers.push_back(tcpserver);
}
/**
 * @brief Sets the node number to be used in the CAN messages
 * @param nn
 */
void  canHandler::setNodeNumber(int nn){
    node_number = nn;
}
/**
 * @brief Sets the can id used in the CAN messages
 * @param canId
 */
void canHandler::setCanId(int canId){
    this->canId = canId;
}
/**
 * @brief Insert a message in the ougoing queue using the actual specified canid
 * @param msg The data to be sent limited by 8 bytes
 * @param msize The message size limited by 8 bytes
 * @param ct The client type sending the message: ED(engine driver client) or GRID (can grid server client)
 * @return
 */
int canHandler::put_to_out_queue(char *msg,int msize,CLIENT_TYPE ct){
    int c = 5; //priority 0101
    c = c << 8;
    c = c | canId;
    return put_to_out_queue(c,msg,msize,ct);
}
/**
 * @brief Insert a CAN frame in the ougoing queue using a customised canid.
 * It multiplexes the message among from the ED clients to the Grid clients
 * @param canid
 * @param msg The data to be sent limited by 8 bytes
 * @param msize The message size limited by 8 bytes
 * @param ct The client type sending the message: ED(engine driver client) or GRID (can grid server client)
 * @return
 */
int canHandler::put_to_out_queue(int canid,char *msg,int msize,CLIENT_TYPE ct){
    int i = 0;
    int j = CAN_MSG_SIZE;
    struct can_frame frame;

    /*
    if (msize == 0){
        if (canid != CAN_RTR_FLAG){
            logger->debug("Can message with size 0 and not RTR. Discarding");
            return 0;
        }
    }
    */
    if (msize < CAN_MSG_SIZE){
        j = msize;
    }
    memset(frame.data , 0 , sizeof(frame.data));
    frame.can_id = canid;
    frame.can_dlc = j;

    for (i = 0;i < j; i++){
        frame.data[i]=msg[i];
    }
    logger->debug("[canHandler] Add message to cbus queue");
    print_frame(&frame,"[canHandler] Insert");
    //thread safe insert
    frameCAN canframe = frameCAN(frame,ct);

    pthread_mutex_lock(&m_mutex);
    //out_msgs.push(frame);
    out_msgs.push(canframe);

    pthread_mutex_unlock(&m_mutex);
    pthread_cond_signal(&m_condv);

    //send to can grid server
    vector<tcpServer*>::iterator server;
    if ((servers.size() > 0) && (ct != CLIENT_TYPE::GRID)){
        for (server = servers.begin();server != servers.end(); server++){
            if ((*server)->getClientType() == CLIENT_TYPE::GRID){
                (*server)->addCanMessage(frame.can_id,(char*)frame.data, frame.can_dlc);
            }
        }
    }
    else{
        print_frame(&frame,"[canHandler] Servers empty or client type is GRID. Not sendind to GRID.");
        //logger->debug("[canHandler] Servers empty or client type is GRID. Not sendind to GRID.");
    }

    return j;
}
/**
 * @brief Add a message to the incoming can queue
 * @param canid
 * @param msg Message received limited by 8 bytes
 * @param msize Message size limited by 8 bytes
 * @param ct Client type: ED or GRID
 * @return
 */
int canHandler::put_to_incoming_queue(int canid,char *msg,int msize,CLIENT_TYPE ct){
    int i = 0;
    int j = CAN_MSG_SIZE;
    struct can_frame frame;

    if (msize < CAN_MSG_SIZE){
        j = msize;
    }
    memset(frame.data , 0 , sizeof(frame.data));
    frame.can_id = canid;
    frame.can_dlc = j;

    for (i = 0;i < j; i++){
        frame.data[i]=msg[i];
    }
    logger->debug("[canHandler] Add message to incoming cbus queue");

    print_frame(&frame,"[canHandler] Insert");
    frameCAN canframe = frameCAN(frame,ct);

    pthread_mutex_lock(&m_mutex_in);

    //in_msgs.push(frame);
    in_msgs.push(canframe);

    pthread_mutex_unlock(&m_mutex_in);
    pthread_cond_signal(&m_condv_in);

    return j;
}
/**
 * @brief Start the major components: CAN interface reader, Consumer of the incoming CAN messages, Consumer of the outgoing CAN messages
 * It sends a start service event
 * @param interface The can interface. Normally can0
 * @return
 */
int canHandler::start(const char* interface){
    logger->debug("[canHandler] Creating socket can for %s",interface);

	if ((canInterface = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0)	{
        logger->error("[canHandler] Unable to create can socket");
		return -1;
	}

	addr.can_family = AF_CAN;
	strcpy(ifr.ifr_name, interface);
	if (ioctl(canInterface, SIOCGIFINDEX, &ifr) < 0){
        logger->error("[canHandler] Failed to start can socket. SIOCGIFINDEX");
        return -1;
	}
	addr.can_ifindex = ifr.ifr_ifindex;

	if (bind(canInterface, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        logger->error("[canHandler] Unable to bind can socket");
		return -1;
	}
	running = 1;
    logger->debug("[canHandler] Can socket successfuly created. Starting the CBUS reader thread");
	pthread_create(&cbusReader,nullptr,canHandler::thread_entry_in,this);

	logger->debug("[canHandler] Starting the queue reader thread");
	pthread_create(&queueReader,nullptr,canHandler::thread_entry_in_reader,this);

	logger->debug("[canHandler] Starting the queue writer thread");
	pthread_create(&queueWriter,nullptr,canHandler::thread_entry_out,this);

	return canInterface;
}
/**
 * @brief Stop all threads. It send an end service event.
 */
void canHandler::stop(){
    send_end_event();
    usleep(200*1000); //wait for the message to be sent
    gl.setval_gpio("0");
    yl.setval_gpio("0");
    running = 0;
}
/**
 * @brief Sends the start service event based on the configuration
 */
void canHandler::send_start_event(){
    char sendframe[CAN_MSG_SIZE];
    int startid = config->getStartEventID();

    memset(sendframe,0,CAN_MSG_SIZE);
    byte Hb, Lb, Hi, Li;
    Lb = node_number & 0xff;
    Hb = (node_number >> 8) & 0xff;
    Li = startid & 0xff;
    Hi = (startid >> 8) & 0xff;
    logger->debug("[canHandler] Sending start event");
    sendframe[0] = OPC_ACON;
    sendframe[1] = Hb;
    sendframe[2] = Lb;
    sendframe[3] = Hi;
    sendframe[4] = Li;
    put_to_out_queue(sendframe,5,CLIENT_TYPE::ED);
}
/**
 * @brief Sends the stop service event based on the configuration
 */
void canHandler::send_end_event(){
    char sendframe[CAN_MSG_SIZE];
    int startid = config->getStartEventID();

    memset(sendframe,0,CAN_MSG_SIZE);
    byte Hb, Lb, Hi, Li;
    Lb = node_number & 0xff;
    Hb = (node_number >> 8) & 0xff;
    Li = startid & 0xff;
    Hi = (startid >> 8) & 0xff;
    logger->debug("[canHandler] Sending start event");
    sendframe[0] = OPC_ACOF;
    sendframe[1] = Hb;
    sendframe[2] = Lb;
    sendframe[3] = Hi;
    sendframe[4] = Li;
    put_to_out_queue(sendframe,5,CLIENT_TYPE::ED);
}
/**
 * @brief Reads the CAN interface and put the frame in a threadsafe queue
 * @param param not used
 */
void canHandler::run_in(void* param){
    struct can_frame frame;
    int nbytes;
    struct msghdr msg;
    char ctrlmsg[CMSG_SPACE(sizeof(struct timeval)) + CMSG_SPACE(sizeof(__u32))];
    struct iovec iov;

    if (canInterface <= 0){
        logger->error("[canHandler] Can socket not initialized");
        return;
    }

    send_start_event();
    iov.iov_base = &frame;
    msg.msg_name = &addr;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = &ctrlmsg;

    logger->debug("[canHandler] Running CBUS reader");
    while (running){
        memset(frame.data , 0 , sizeof(frame.data));

        /* these settings may be modified by recvmsg() */
        iov.iov_len = sizeof(frame);
        msg.msg_namelen = sizeof(addr);
        msg.msg_controllen = sizeof(ctrlmsg);
        msg.msg_flags = 0;

        //nbytes = read(canInterface, &frame, sizeof(frame));
        nbytes = recvmsg(canInterface, &msg , 0);
        if (nbytes < 0)
        {
            logger->error("[canHandler] Can not read CBUS");
        }
        else{
            frameCAN canframe = frameCAN(frame,CLIENT_TYPE::CBUS);
            pthread_mutex_lock(&m_mutex_in);
            //in_msgs.push(frame);
            in_msgs.push(canframe);
            pthread_mutex_unlock(&m_mutex_in);
            pthread_cond_signal(&m_condv_in);
        }
    }
    logger->debug("[canHandler] Shutting down the CBUS socket");
    close(canInterface);
}

/**
 * @brief Consumes the CAN queue. It checks for the following OPC to handle:
 * QNN RQMN RQNP SNN ENUM HLT BON BOOT ARST CANID NVSET RQNPN NVRD
 * It deals with autoenumeration and also distributes the can frame to the tcp servers.
 * @param param
 */
void canHandler::run_queue_reader(void* param){
    struct can_frame frame;
    byte opc;
    bool stdframe = true;
    frameCAN canframe;

    logger->debug("[canHandler] Running CBUS queue reader");

    vector<tcpServer*>::iterator server;

    while (running){

        if (!in_msgs.empty()){
            pthread_mutex_lock(&m_mutex_in);
            //frame = in_msgs.front();
            canframe = in_msgs.front();
            in_msgs.pop();
            pthread_mutex_unlock(&m_mutex_in);

            frame = canframe.getFrame();
            if ((frame.can_id & CAN_EFF_FLAG) == CAN_EFF_FLAG){
                stdframe = false;
                print_frame(&frame,"[canHandler] Received extended frame");
            }
            else{
                stdframe = true;
                print_frame(&frame,"[canHandler] Received standard frame");
            }

            //finish auto enum
            if (auto_enum_mode && stdframe){
                if (frame.can_dlc == 0){
                    finishSelfEnum(frame.can_id);
                }
            }
            /*
            * Check if some other node is doing auto enum
            * and answer with out canid
            */
            if (((frame.can_id & CAN_RTR_FLAG) == CAN_RTR_FLAG) && stdframe){
                struct can_frame frame_resp;
                memset(frame_resp.data , 0 , sizeof(frame_resp.data));
                frame_resp.can_id = canId & 0x7f;
                frame_resp.can_dlc = 0;
                frame_resp.data[0] = canId;
                put_to_out_queue(frame_resp.can_id,(char*)frame_resp.data,0,CLIENT_TYPE::ED);

            }
            else{
                //Handle cbus
                if (stdframe){
                    opc = frame.data[0];
                    if (opc == OPC_QNN ||
                        opc == OPC_RQMN ||
                        opc == OPC_RQNP ||
                        opc == OPC_SNN ||
                        opc == OPC_ENUM ||
                        opc == OPC_HLT ||
                        opc == OPC_BON ||
                        opc == OPC_BOOT ||
                        opc == OPC_ARST ||
                        opc == OPC_CANID ||
                        opc == OPC_NVSET ||
                        opc == OPC_RQNPN ||
                        opc == OPC_NVRD ||
                        opc == OPC_ASON){
                        handleCBUSEvents(canframe);
                    }
                }
            }

            if (servers.size() > 0){
                for (server = servers.begin();server != servers.end(); server++){
                    //do not send message from GRID to GRID
                    if (canframe.getClientType() == CLIENT_TYPE::GRID){
                        if ((*server)->getClientType() != canframe.getClientType()){
                            (*server)->addCanMessage(frame.can_id,(char*)frame.data, frame.can_dlc);
                        }
                        else{
                            print_frame(&frame,"[canHandler] Droping message from GRID to GRID");
                            //logger->debug("[canHandler] Droping message from GRID to GRID");
                        }
                    }
                    else{
                        (*server)->addCanMessage(frame.can_id,(char*)frame.data, frame.can_dlc);
                    }

                }
            }
        }
        //check button pressed
        //finish auto enum
        if (auto_enum_mode){
            finishSelfEnum(0);

            if (!auto_enum_mode){

            	//send RQNN
            	char sendframe[CAN_MSG_SIZE];
        	memset(sendframe,0,CAN_MSG_SIZE);

        	byte Lb,Hb;
        	Lb = node_number & 0xff;
        	Hb = (node_number >> 8) & 0xff;
		logger->info("[canHandler] Doing request node number afer auto enum. Entering in setup mode");

            	sendframe[0]=OPC_RQNN;
            	sendframe[1] = Hb;
            	sendframe[2] = Lb;
            	setup_mode = true;
            	put_to_out_queue(sendframe,3,CLIENT_TYPE::ED);
             }

        }
        doPbLogic();
        usleep(3000);
    }
    logger->debug("[canHandler] Stopping the queue reader");
}
/**
 * @brief Print the can frame in a readable format
 * @param frame The can frame message
 * @param message A message to add on the output
 */
void canHandler::print_frame(can_frame *frame,string message){

    logger->debug("%s Can Data : [%03x] [%d] data: %02x %02x %02x %02x %02x %02x %02x %02x"
                ,message.c_str()
                ,frame->can_id , frame->can_dlc
                ,frame->data[0],frame->data[1],frame->data[2],frame->data[3]
                ,frame->data[4],frame->data[5],frame->data[6],frame->data[7]);
}

/**
 * @brief It runs as a thread and it is responsible to consume the outgoing CAN frame queue and send to the CAN interface.
 * @param param not used
 */
void canHandler::run_out(void* param){
    struct can_frame frame;
    int nbytes;
    frameCAN canframe;

    logger->debug("[canHandler] Running CBUS queue writer");

    while (running){
        if (!out_msgs.empty()){
            pthread_mutex_lock(&m_mutex);
            //frame = out_msgs.front();
            canframe = out_msgs.front();
            out_msgs.pop();
            pthread_mutex_unlock(&m_mutex);
            frame = canframe.getFrame();

            if (cbus_stopped){
                logger->warn("[canHandler] CBUS stopped. Discarding outgoing message");
                continue;
            }
            nbytes = write(canInterface, &frame, CAN_MTU);
            print_frame(&frame,"[canHandler] Sent [" + to_string(nbytes) + "]");
            //logger->debug("Sent %d bytes to CBUS",nbytes);
            if (nbytes != CAN_MTU){
                logger->debug("[canHandler] Problem on sending the CBUS, bytes transfered %d, supposed to transfer %d", nbytes, CAN_MTU);
            }
        }
        usleep(3000);
    }
    logger->debug("[canHandler] Stopping the queue writer");
}

/**
 * @brief Starts the self enumeration procedure
 */
void canHandler::doSelfEnum(){
    //start can enumeration
    logger->debug("[canHandler] Starting can enumeration");
    auto_enum_mode = true;
    struct can_frame frame;
    memset(frame.data , 0 , sizeof(frame.data));
    byte c = canId & 0x7f;
    frame.can_id = CAN_RTR_FLAG | c;
    frame.can_dlc = 0;
    frame.data[0] = canId;
    sysTimeMS_start = time(0)*1000;
    put_to_out_queue(frame.can_id,(char*)frame.data,0,CLIENT_TYPE::ED);
}

/**
 * @brief Finishes the auto enumeration
 * @param id is the a CAN id
 */
void canHandler::finishSelfEnum(int id){
    sysTimeMS_end = time(0)*1000;
    if (id!=0) {
        byte ct;
        ct = id & 0x7f;
        canids.push_back(ct);
    }
    if ((sysTimeMS_end - sysTimeMS_start) > WAIT_ENUM){
        logger->debug("[canHandler] Finishing auto enumeration.");
        auto_enum_mode = false;
        //sort and get the smallest free
        if (canids.size()>0){
            int c,n;
            canId = 0;
            sort(canids.begin(),canids.end());
            c = canids.front();
            n = 1;
            while (canId == 0){
                if (n != c){
                    canId = n;
                }
                else{
                    n++;
                    if (canids.size() > 0){
                        c = canids.front();
                    }
                    else {
                        canId = n;
                    }
                }
            }
        }
        else{
            canId = 1;
        }
        config->setCanID(canId);
        logger->debug("[canHandler] New canid is %d", canId);
    }
}
/**
 * @brief Handle the specific CBUS configuration events
 * @param frame The CAN frame
 */
//void canHandler::handleCBUSEvents(struct can_frame frame){
void canHandler::handleCBUSEvents(frameCAN canframe){

    char sendframe[CAN_MSG_SIZE];
    memset(sendframe,0,CAN_MSG_SIZE);
    byte Hb,Lb;
    int tnn;
    SCRIPT_ACTIONS status = NONE;
    struct can_frame frame = canframe.getFrame();
    print_frame(&frame,"[canHandler] Handling CBUS event");

    switch (frame.data[0]){
    case OPC_QNN:
    {
        if (setup_mode) return;
        Lb = node_number & 0xff;
        Hb = (node_number >> 8) & 0xff;
        logger->debug("[canHandler] Sending response for QNN.");
        sendframe[0] = OPC_PNN;
        sendframe[1] = Hb;
        sendframe[2] = Lb;
        sendframe[3] = MANU_MERG;
        sendframe[4] = MID;
        sendframe[5] = MFLAGS;
        put_to_out_queue(sendframe, 6, CLIENT_TYPE::CBUS);

        break;
    }
    case OPC_RQNP:
    {
        if (!setup_mode) return;
        logger->debug("[canHandler] Sending response for RQNP.");
        sendframe[0] = OPC_PARAMS;
        sendframe[1] = MANU_MERG;
        sendframe[2] = MSOFT_MIN_VERSION;
        sendframe[3] = MID;
        sendframe[4] = 0;
        sendframe[5] = 0;
        sendframe[6] = config->getNumberOfNVs();//TODO
        sendframe[7] = MSOFT_VERSION;
        put_to_out_queue(sendframe, 8, CLIENT_TYPE::CBUS);

        break;
    }
    case OPC_RQMN:
    {
        if (!setup_mode) return;
        logger->debug("[canHandler] Sending response for NAME.");
        sendframe[0] = OPC_NAME;
        sendframe[1] = 'P';
        sendframe[2] = 'i';
        sendframe[3] = 'W';
        sendframe[4] = 'i';
        sendframe[5] = ' ';
        sendframe[6] = ' ';
        sendframe[7] = ' ';
        put_to_out_queue(sendframe, 8, CLIENT_TYPE::CBUS);
        break;
    }
    case OPC_RQNPN:
    {
        Lb = frame.data[2];
        Hb = frame.data[1];
        tnn = Hb;
        tnn = (tnn << 8) | Lb;
        byte p;
        if (tnn != node_number){
            logger->debug("[canHandler] RQNPN is for another node. My nn: %d received nn: %d", node_number,tnn);
            return;
        }
        if (frame.data[3] > NODE_PARAMS_SIZE) {
            //index invalid
            sendframe[0] = OPC_CMDERR;
            sendframe[1] = Hb;
            sendframe[2] = Lb;
            sendframe[3] = 9;
            put_to_out_queue(sendframe, 4, CLIENT_TYPE::CBUS);
            return;
        }
        p = config->getNodeParameter(frame.data[3]);
        if (frame.data[3] == 0){
            p = NODE_PARAMS_SIZE;
        }
        sendframe[0] = OPC_PARAN;
        sendframe[1] = Hb;
        sendframe[2] = Lb;
        sendframe[3] = frame.data[3];
        sendframe[4] = p;
        put_to_out_queue(sendframe, 5, CLIENT_TYPE::CBUS);
        break;
    }
    case OPC_NVRD:
    {
        Lb = frame.data[2];
        Hb = frame.data[1];
        tnn = Hb;
        tnn = (tnn << 8) | Lb;
        if (tnn != node_number){
            logger->debug("[canHandler] NVRD is for another node. My nn: %d received nn: %d", node_number,tnn);
            return;
        }
        if (frame.data[3] > config->getNumberOfNVs() || frame.data[3] == 0) {
            //index invalid
            sendframe[0] = OPC_CMDERR;
            sendframe[1] = Hb;
            sendframe[2] = Lb;
            sendframe[3] = 9;
            put_to_out_queue(sendframe, 4, CLIENT_TYPE::CBUS);
            logger->debug("[canHandler] NVRD Invalid index %d", frame.data[3]);
            return;
        }
        sendframe[0] = OPC_NVANS;
        sendframe[1] = Hb;
        sendframe[2] = Lb;
        sendframe[3] = frame.data[3];
        sendframe[4] = config->getNV(frame.data[3]);
        put_to_out_queue(sendframe, 5, CLIENT_TYPE::CBUS);
        logger->debug("[canHandler] NVRD processed. Sent NVANS");
        break;
    }

    case OPC_NVSET:
    {
        Lb = frame.data[2];
        Hb = frame.data[1];
        tnn = Hb;
        tnn = (tnn << 8) | Lb;
        if (tnn != node_number){
            logger->debug("[canHandler] NVSET is for another node. My nn: %d received nn: %d", node_number,tnn);
            return;
        }
        if (frame.data[3] > config->getNumberOfNVs() || frame.data[3] == 0) {
            //index invalid
            sendframe[0] = OPC_CMDERR;
            sendframe[1] = Hb;
            sendframe[2] = Lb;
            sendframe[3] = 9;
            put_to_out_queue(sendframe,4 , CLIENT_TYPE::CBUS);
            logger->debug("[canHandler] NVSET Invalid index %d", frame.data[3]);
            return;
        }

        //1 error, 2 reconfigure , 3 restart the service
        int st = config->setNV(frame.data[3],frame.data[4]);
        if (st == 1){
            sendframe[0] = OPC_CMDERR;
            sendframe[1] = Hb;
            sendframe[2] = Lb;
            sendframe[3] = 9;
            put_to_out_queue(sendframe, 4, CLIENT_TYPE::CBUS);
            logger->debug("[canHandler] NVSET failed. Sent Err");
            status = NONE;
        }
        else{
            sendframe[0] = OPC_WRACK;
            sendframe[1] = Hb;
            sendframe[2] = Lb;
            put_to_out_queue(sendframe, 3, CLIENT_TYPE::CBUS);
            logger->debug("[canHandler] NVSET ok. Sent wrack");
        }

        if (st == 2 || st == 3){
            if (st == 2) status = CONFIGURE;
            if (st == 3) status = RESTART;
            restart_module(status);
        }

        break;
    }
    case OPC_SNN:
    {

        if (!setup_mode) return;

        Lb = frame.data[2];
        Hb = frame.data[1];
        tnn = Hb;
        tnn = (tnn << 8) | Lb;

        logger->debug("[canHandler] Saving node number %d.",tnn);
        if (config->setNodeNumber(tnn)){
            logger->debug("[canHandler] Save node number success.");
            node_number = tnn;
            Lb = node_number & 0xff;
            Hb = (node_number >> 8) & 0xff;
            sendframe[0] = OPC_NNACK;
            sendframe[1] = Hb;
            sendframe[2] = Lb;
            put_to_out_queue(sendframe, 3, CLIENT_TYPE::CBUS);
        }
        else{
            logger->error("[canHandler] Save node number failed. Maintaining the old one");
            Lb = node_number & 0xff;
            Hb = (node_number >> 8) & 0xff;
            sendframe[0] = OPC_CMDERR;
            sendframe[1] = Hb;
            sendframe[2] = Lb;
            sendframe[3] = 5;
            put_to_out_queue(sendframe, 4, CLIENT_TYPE::CBUS);
        }
        setup_mode = false;
        blinking = false;
        gl.setval_gpio("0");
        yl.setval_gpio("1");
        logger->info("Node was in SLIM. Setting to FLIM");
        config->setNodeMode(1); //FLIM
        logger->info("[canHandler] Finished setup. New node number is %d" , node_number);

        break;
    }
    case OPC_CANID:
    {
        if (setup_mode) return;
        logger->debug("[canHandler] Received set CANID.");
        Lb = frame.data[2];
        Hb = frame.data[1];
        tnn = Hb;
        tnn = (tnn << 8) | Lb;
        if (tnn != node_number){
            logger->debug("[canHandler] Set CANID is for another node. My nn: %d received nn: %d", node_number,tnn);
            return;
        }
        int tcanid;
        tcanid = frame.data[3];
        if (tcanid < 1 || tcanid > 99){
            logger->debug("[canHandler] CANID [%d] out of range 1-99. Sending error.", tcanid);
            Lb = node_number & 0xff;
            Hb = (node_number >> 8) & 0xff;
            sendframe[0] = OPC_CMDERR;
            sendframe[1] = Hb;
            sendframe[2] = Lb;
            sendframe[3] = 7;
            put_to_out_queue(sendframe, 4, CLIENT_TYPE::CBUS);
            return;
        }
        canId = tcanid;
        logger->debug("[canHandler] Saving new CANID %d",canId);
        if (!config->setCanID(canId)){
            logger->error("[canHandler] Failed to save canid %d",canId);
        }

        break;
    }
    case OPC_ASON:
    {
        Lb = frame.data[4];
        Hb = frame.data[3];
        tnn = Hb;
        tnn = (tnn << 8) | Lb;
        logger->debug("[canHandler] Received set ASON %d", tnn);
        if (tnn == config->getShutdownCode()){
            logger->debug("[canHandler] Shuting down the node.");
            restart_module(SHUTDOWN);
            return;
        }

        break;
    }
    case OPC_ENUM:
    {
        if (setup_mode) return;
        //get node number
        int nn;
        nn = frame.data[1];
        nn = (nn << 8) | frame.data[2];
        logger->debug("[canHandler] OPC_ENUM node number %d",nn);
        doSelfEnum();
        break;
    }
    case OPC_HLT:
    {
        if (setup_mode) return;
        logger->info("[canHandler] Stopping CBUS");
        cbus_stopped = true;
        break;
    }
    case OPC_BON:
    {
        if (setup_mode) return;
        logger->info("[canHandler] Enabling CBUS");
        cbus_stopped = false;
        break;
    }
    case OPC_ARST:
    {
        if (setup_mode) return;
        logger->info("[canHandler] Enabling CBUS");
        cbus_stopped = false;
        break;
    }
    case OPC_BOOT:
    {
        if (setup_mode) return;
        logger->info("[canHandler] Rebooting");
        break;
    }
    }
}
/**CAN_EFF_FLAG
 * @brief Restarts the module depending on the @status parameter
 * @param status 0 to 3. 0 and 1 does nothing.
 * 2 reconfigure the module. It also forces a reboot.
 * 3 restarts the canpi and webserver services
 */
void canHandler::restart_module(SCRIPT_ACTIONS action){
    string command;
    //MTA*<;>*

    if (action == CONFIGURE){
        command = "/etc/init.d/start_canpi.sh configure";
    }
    else if (action == RESTART){
        command = "/etc/init.d/start_canpi.sh restart";
    }
    else if (action == SHUTDOWN){
        command = "/etc/init.d/start_canpi.sh shutdown";
    }
    else{
        return;
    }
    //all parameters saved, we can restart or reconfigure the module
    logger->debug("[canHandler] Restart after new configuration %d", action);
    logger->debug("[canHandler] Stopping all servers");
    vector<tcpServer*>::iterator server;
    if (servers.size() > 0){
        for (server = servers.begin();server != servers.end(); server++){
            (*server)->stop();
        }
    }
    usleep(1000*1000);
    if(fork() == 0){
        logger->info("[canHandler] Restarting the module. [%s]", command.c_str());
        system(command.c_str());
        exit(0);
    }else{
        logger->info("[canHandler] Main module stopping the services");
    }
}

/**
 * @brief Handles the push button behaviour
 */

void canHandler::doPbLogic(){
    string pbstate;
    string ledstate;

    if (blinking){
        if (((time(0)*1000) - ledtime) > BLINK_INTERVAL){
            yl.getval_gpio(ledstate);
            if (ledstate == "0"){
                yl.setval_gpio("1");
            }
            else{
                yl.setval_gpio("0");
            }
            ledtime = time(0) * 1000;
        }
    }

    pb.getval_gpio(pbstate);
    if (pbstate == "0"){
        //button pressed
        //logger->debug("Button pressed.");
        if (!pb_pressed){
            //button was not pressed. start timer
            nnPressTime = time(0)*1000;
            pb_pressed = true;
            logger->debug("[canHandler] Button pressed. Timer start %le",nnPressTime );
        }
        else{
            ledtime = time(0)*1000;
            if (((ledtime - nnPressTime) >= NN_PB_TIME) && !blinking){
                blinking = true;
                gl.setval_gpio("1");
            }
        }
        return;
    }

    //button was pressed and now released
    if (pb_pressed){
        nnReleaseTime = time(0)*1000;
        pb_pressed = false;
        logger->debug("[canHandler] Button released. Timer end [%le] difference [%lf]",nnReleaseTime, nnReleaseTime - nnPressTime );

        char sendframe[CAN_MSG_SIZE];
        memset(sendframe,0,CAN_MSG_SIZE);

        byte Lb,Hb;
        Lb = node_number & 0xff;
        Hb = (node_number >> 8) & 0xff;

        //check if node number request
        if ((nnReleaseTime - nnPressTime) >= NN_PB_TIME){

            if (config->getNodeMode() == 1){//change from FLIM to SSLIM
                logger->info("Node was in FLIM. Setting to SLIM");
            	config->setNodeMode(0); //SLIM
            	gl.setval_gpio("1");
            	yl.setval_gpio("0");

            	sendframe[0] = OPC_NNREL;
            	sendframe[1] = Hb;
            	sendframe[2] = Lb;
            	setup_mode = false;
                blinking = false;
            	put_to_out_queue(sendframe, 3, CLIENT_TYPE::ED);
            	node_number = DEFAULT_NN;
            	config->setNodeNumber(DEFAULT_NN);
            	return;

           }


            //send RQNN
            logger->info("[canHandler] Doing request node number. Entering in setup mode");

            sendframe[0]=OPC_RQNN;
            sendframe[1] = Hb;
            sendframe[2] = Lb;
            setup_mode = true;
            put_to_out_queue(sendframe, 3, CLIENT_TYPE::ED);
            return;
        }
        //check if auto enum request
        if ((nnReleaseTime - nnPressTime) >= AENUM_PB_TIME){
            doSelfEnum();
             //send RQNN
            /*
            logger->info("[canHandler] Doing request node number. Entering in setup mode");

            sendframe[0]=OPC_RQNN;
            sendframe[1] = Hb;
            sendframe[2] = Lb;
            setup_mode = true;
            put_to_out_queue(sendframe, 3, CLIENT_TYPE::ED);
            */
            return;
        }
        if (setup_mode || blinking){
            logger->debug("[canHandler] Leaving setup modeCAN");
            setup_mode = false;
            blinking = false;
            if (config->getNodeMode() == 1 ) { //FLIM
            	gl.setval_gpio("0");
            	yl.setval_gpio("1");
            	logger->info("Setting to FLIM");
	    }
	    else{
            	gl.setval_gpio("1");
            	yl.setval_gpio("0");
            	logger->info("Setting to SLIM");
	    }
        }
    }
}
