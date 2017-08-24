/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** main.cpp
** 
** V4L2 RTSP streamer                                                                 
**                                                                                    
** H264 capture using V4L2                                                            
** RTSP using live555                                                                 
**                                                                                    
** -------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/time.h>

#include <sstream>

// libv4l2
#include <linux/videodev2.h>

// live555
#include <BasicUsageEnvironment.hh>
#include <GroupsockHelper.hh>

// project
#include "V4l2Device.h"
#include "V4l2Capture.h"
#include "AitXU.h"

#include "H264_V4l2DeviceSource.h"
#include "ServerMediaSubsession.h"

#define VERSION "0.0.5"

#ifdef DEBUG
#define LOG_DEBUG(X) std::cout << X << std::endl
#else
#define LOG_DEBUG(X)
#endif

RTSPServer* rtspServer;

// -----------------------------------------
//    signal handler
// -----------------------------------------
char quit = 0;
void sighandler(int n)
{
	printf("SIGINT\n");
	//Medium::close(rtspServer);
	quit =1;
}


// -----------------------------------------
//    create UserAuthenticationDatabase for RTSP server
// -----------------------------------------
UserAuthenticationDatabase* createUserAuthenticationDatabase(const std::list<std::string> & userPasswordList, const char* realm)
{
	UserAuthenticationDatabase* auth = NULL;
	if (userPasswordList.size() > 0)
	{
		auth = new UserAuthenticationDatabase(realm, (realm != NULL) );

		std::list<std::string>::const_iterator it;
		for (it = userPasswordList.begin(); it != userPasswordList.end(); ++it)
		{
			std::istringstream is(*it);
			std::string user;
			getline(is, user, ':');
			std::string password;
			getline(is, password);
			auth->addUserRecord(user.c_str(), password.c_str());
		}
	}

	return auth;
}
	
// -----------------------------------------
//    add an RTSP session
// -----------------------------------------
int addSession(RTSPServer* rtspServer, const std::string & sessionName, const std::list<ServerMediaSubsession*> & subSession)
{
	int nbSubsession = 0;
	if (subSession.empty() == false)
	{
		UsageEnvironment& env(rtspServer->envir());
		ServerMediaSession* sms = ServerMediaSession::createNew(env, sessionName.c_str());
		if (sms != NULL)
		{
			std::list<ServerMediaSubsession*>::const_iterator subIt;
			for (subIt = subSession.begin(); subIt != subSession.end(); ++subIt)
			{
				sms->addSubsession(*subIt);
				nbSubsession++;
			}

			rtspServer->addServerMediaSession(sms);

			char* url = rtspServer->rtspURL(sms);
			if (url != NULL)
			{
				std::cout << "Play this stream using the URL \"" << url << "\"" << std::endl;
				delete[] url;
			}
		}
	}
	return nbSubsession;
}

void showHelp(char * argv0, V4L2DeviceParameters * dparam, int timeout, int queueSize, unsigned short rtspPort, const char * url)
{
	std::cout << argv0 << " [-Q queueSize] [-I interface] [-P RTSP port] [-u unicast url] [-c]"         << std::endl;
	std::cout << "\t          [-t timeout] [-T] [-S[duration]] [-W width] [-H height] [-F fps]"         << std::endl;
	std::cout << "\t          [-a [-m] [-f] [-i mode]] [device]"                                        << std::endl;
	std::cout << "\t -Q length : Number of frame queue  (default " << queueSize << ")"                  << std::endl;

	std::cout << "\t RTSP/RTP options :"                                                                << std::endl;
	std::cout << "\t -I addr   : RTSP interface (default autodetect)"                                   << std::endl;
	std::cout << "\t -P port   : RTSP port (default " << rtspPort << ")"                                << std::endl;
	std::cout << "\t -U user:password : RTSP user and password"                                         << std::endl;
	std::cout << "\t -R realm  : use md5 password 'md5(<username>:<realm>:<password>')"                 << std::endl;
	std::cout << "\t -u url    : unicast url (default " << url << ")"                                   << std::endl;
	std::cout << "\t -c        : don't repeat config (default repeat config before IDR frame)"          << std::endl;
	std::cout << "\t -t timeout: RTCP expiration timeout in seconds (default " << timeout << ")"        << std::endl;

	std::cout << "\t V4L2 options :"                                                                    << std::endl;
	std::cout << "\t -W width  : V4L2 capture width (default " << dparam->m_width << ")"                 << std::endl;
	std::cout << "\t -H height : V4L2 capture height (default " << dparam->m_height << ")"               << std::endl;
	std::cout << "\t -F fps    : V4L2 capture framerate (default " << dparam->m_fps << ")"               << std::endl;

	std::cout << "\t AIT Extension :"                                                                   << std::endl;
	std::cout << "\t -a        : enable AIT Extension"                                                  << std::endl;
	std::cout << "\t -f        : Flip vertical"                                                         << std::endl;
	std::cout << "\t -m        : Mirror horizontal"                                                     << std::endl;
	std::cout << "\t -i mode   : IR Cut Mode ([0] Default; [1] Day mode; [2] Night mode)"               << std::endl;

	std::cout << "\t Devices :"                                                                         << std::endl;
	std::cout << "\t [V4L2 device] : V4L2 capture device (default " << dparam->m_devName << ")"          << std::endl;
}

// -----------------------------------------
//    entry point
// -----------------------------------------
int main(int argc, char** argv) 
{
	// default parameters´
	int queueSize = 10;
	unsigned short rtspPort = 8554;
	std::string url = "unicast";
	bool repeatConfig = true;
	int timeout = 65;
	const char* realm = NULL;
	std::list<std::string> userPasswordList;

	V4L2DeviceParameters dparam(
		/*.m_devName =*/ "/dev/video0",
		/*.m_format =*/ V4L2_PIX_FMT_H264,
		/*.m_width =*/ 640,
		/*.m_height =*/ 480,
		/*.m_fps =*/ 25,
		/*.m_ait =*/ false,
		/*.m_aitMirrFlip =*/ AIT_FM_NORMAL,
		/*.m_aitIRCutMode =*/ AIT_IR_DEFAULT
	);

	// decode parameters
	int c = 0;
	while ((c = getopt (argc, argv, "Q:" "I:P:u:ct:" "R:U:" "F:W:H:" "afmi:" "V")) != -1)
	{
		switch (c)
		{
			case 'Q':	queueSize  = atoi(optarg); break;
			
			// RTSP/RTP
			case 'I':	ReceivingInterfaceAddr  = inet_addr(optarg); break;
			case 'P':	rtspPort                = atoi(optarg); break;
			case 'u':	url                     = optarg; break;
			case 'c':	repeatConfig            = false; break;
			case 't':	timeout                 = atoi(optarg); break;
			
			// users
			case 'R':	realm                   = optarg; break;
			case 'U':	userPasswordList.push_back(optarg); break;
			
			// V4L2
			case 'F':	dparam.m_fps			= atoi(optarg); break;
			case 'W':	dparam.m_width			= atoi(optarg); break;
			case 'H':	dparam.m_height			= atoi(optarg); break;
			
			// AIT Extension
			case 'a':	dparam.m_ait			= true; break;
			case 'f':	dparam.m_aitMirrFlip	|= AIT_FM_FLIP; break;
			case 'm':	dparam.m_aitMirrFlip	|= AIT_FM_MIRROR; break;
			case 'i':	dparam.m_aitIRCutMode	= atoi(optarg); break;

			// version
			case 'V':
				std::cout << VERSION << std::endl;
				exit(0);
			break;
			
			// help
			case 'h':
			default:
			{
				showHelp(argv[0], &dparam, timeout, queueSize, rtspPort, url.c_str());
				exit(0);
			}
		}
	}
	if (optind < argc)
	{
		dparam.m_devName = argv[optind];
	}

	// create live555 environment
	TaskScheduler* scheduler = BasicTaskScheduler::createNew();
	UsageEnvironment* env = BasicUsageEnvironment::createNew(*scheduler);
	UserAuthenticationDatabase* auth = createUserAuthenticationDatabase(userPasswordList, realm);

	// create RTSP server
	rtspServer = RTSPServer::createNew(*env, rtspPort, auth, timeout);
	if (rtspServer == NULL)
	{
		std::cerr << "Failed to create RTSP server: " << env->getResultMsg() << std::endl;

		env->reclaim();
		delete scheduler;

		return 1;
	}

	// Init video capture
	std::cout << "Create V4L2 Source..." << dparam.m_devName << std::endl;
	V4l2Capture* videoCapture = V4l2Capture::create(dparam, V4l2Access::IOTYPE_MMAP);
	if (videoCapture == NULL)
	{
		std::cerr << "Failed to create V4L2 source " << std::endl;

		Medium::close(rtspServer);
		env->reclaim();
		delete scheduler;

		return 1;
	}

	std::cout << "Create Source ..." << dparam.m_devName << std::endl;
	FramedSource* videoSource = H264_V4L2DeviceSource::createNew(*env, new DeviceCaptureAccess<V4l2Capture>(videoCapture), -1, queueSize, true, repeatConfig, false);
	if (videoSource == NULL)
	{
		std::cerr << "Unable to create source for device " << dparam.m_devName << std::endl;

		delete videoCapture;
		Medium::close(rtspServer);
		env->reclaim();
		delete scheduler;

		return 1;
	}

	// extend buffer size if needed
	if (videoCapture->getBufferSize() > OutPacketBuffer::maxSize)
	{
		OutPacketBuffer::maxSize = videoCapture->getBufferSize();
	}


	StreamReplicator* videoReplicator = StreamReplicator::createNew(*env, videoSource, false);
	if (videoReplicator == NULL)
	{
		std::cerr << "Unable to create replicator for device " << dparam.m_devName << std::endl;

		delete videoCapture;
		Medium::close(rtspServer);
		env->reclaim();
		delete scheduler;

		return 1;

	}

	// Create Unicast Session
	std::list<ServerMediaSubsession*> subSession;
	subSession.push_back(UnicastServerMediaSubsession::createNew(*env, videoReplicator, "video/H264"));

	if (addSession(rtspServer, url, subSession) > 0)
	{
		// main loop
		signal(SIGINT, sighandler);
		env->taskScheduler().doEventLoop(&quit);
		std::cout << "Exiting...." << std::endl;
	}
	
	Medium::close(rtspServer);
	env->reclaim();
	delete scheduler;
	
	return 0;
}



