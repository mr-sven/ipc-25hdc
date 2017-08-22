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
#include "V4l2Output.h"

#include "H264_V4l2DeviceSource.h"
#include "ServerMediaSubsession.h"

#define VERSION "0.0.3"

#ifdef DEBUG
#define LOG_DEBUG(X) std::cout << X << std::endl
#else
#define LOG_DEBUG(X)
#endif

// -----------------------------------------
//    signal handler
// -----------------------------------------
char quit = 0;
void sighandler(int n)
{ 
	printf("SIGINT\n");
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
//    create FramedSource server
// -----------------------------------------
FramedSource* createFramedSource(UsageEnvironment* env, int format, DeviceInterface* videoCapture, int outfd, int queueSize, bool useThread, bool repeatConfig)
{
	FramedSource* source = NULL;
	if (format == V4L2_PIX_FMT_H264)
	{
		source = H264_V4L2DeviceSource::createNew(*env, videoCapture, outfd, queueSize, useThread, repeatConfig, false);
	}
	else
	{
		source = V4L2DeviceSource::createNew(*env, videoCapture, outfd, queueSize, useThread);
	}
	return source;
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

// -----------------------------------------
//    convert V4L2 pix format to RTP mime
// -----------------------------------------
std::string getRtpFormat(int format, bool muxTS)
{
	std::string rtpFormat;
	if (muxTS)
	{
		rtpFormat = "video/MP2T";
	}
	else
	{
		switch(format)
		{	
			case V4L2_PIX_FMT_H264 : rtpFormat = "video/H264"; break;
			case V4L2_PIX_FMT_MJPEG: rtpFormat = "video/JPEG"; break;
			case V4L2_PIX_FMT_JPEG : rtpFormat = "video/JPEG"; break;
			case V4L2_PIX_FMT_VP8  : rtpFormat = "video/VP8" ; break;
			case V4L2_PIX_FMT_VP9  : rtpFormat = "video/VP9" ; break;
		}
	}
	
	return rtpFormat;
}

// -----------------------------------------
//    entry point
// -----------------------------------------
int main(int argc, char** argv) 
{
	// default parameters
	const char *dev_name = "/dev/video0";
	int format = V4L2_PIX_FMT_H264;
	int width = 640;
	int height = 480;
	int queueSize = 10;
	int fps = 25;
	unsigned short rtspPort = 8554;
	std::string url = "unicast";
	bool useThread = true;
	bool repeatConfig = true;
	int timeout = 65;
	const char* realm = NULL;
	std::list<std::string> userPasswordList;

	// decode parameters
	int c = 0;     
	while ((c = getopt (argc, argv, "Q:" "I:P:u:ct:" "R:U:" "F:W:H:" "V")) != -1)
	{
		switch (c)
		{
			case 'Q':	queueSize  = atoi(optarg); break;
			
			// RTSP/RTP
			case 'I':       ReceivingInterfaceAddr  = inet_addr(optarg); break;
			case 'P':	rtspPort                = atoi(optarg); break;
			case 'u':	url                     = optarg; break;
			case 'c':	repeatConfig            = false; break;
			case 't':	timeout                 = atoi(optarg); break;
			
			// users
			case 'R':       realm                   = optarg; break;
			case 'U':       userPasswordList.push_back(optarg); break;
			
			// V4L2
			case 'F':	fps       = atoi(optarg); break;
			case 'W':	width     = atoi(optarg); break;
			case 'H':	height    = atoi(optarg); break;
			
			// version
			case 'V':	
				std::cout << VERSION << std::endl;
				exit(0);			
			break;
			
			// help
			case 'h':
			default:
			{
				std::cout << argv[0] << " [-Q queueSize]"                                                           << std::endl;
				std::cout << "\t          [-I interface] [-P RTSP port] [-u unicast url] [-c] [-t timeout] [-T] [-S[duration]]" << std::endl;
				std::cout << "\t          [-W width] [-H height] [-F fps] [device]"                                 << std::endl;
				std::cout << "\t -Q length : Number of frame queue  (default "<< queueSize << ")"                   << std::endl;
				
				std::cout << "\t RTSP/RTP options :"                                                                << std::endl;
				std::cout << "\t -I addr   : RTSP interface (default autodetect)"                                   << std::endl;
				std::cout << "\t -P port   : RTSP port (default "<< rtspPort << ")"                                 << std::endl;
				std::cout << "\t -U user:password : RTSP user and password"                                         << std::endl;
				std::cout << "\t -R realm  : use md5 password 'md5(<username>:<realm>:<password>')"                 << std::endl;
				std::cout << "\t -u url    : unicast url (default " << url << ")"                                   << std::endl;
				std::cout << "\t -c        : don't repeat config (default repeat config before IDR frame)"          << std::endl;
				std::cout << "\t -t timeout: RTCP expiration timeout in seconds (default " << timeout << ")"        << std::endl;
				
				std::cout << "\t V4L2 options :"                                                                    << std::endl;
				std::cout << "\t -W width  : V4L2 capture width (default "<< width << ")"                           << std::endl;
				std::cout << "\t -H height : V4L2 capture height (default "<< height << ")"                         << std::endl;
				std::cout << "\t -F fps    : V4L2 capture framerate (default "<< fps << ")"                         << std::endl;
				
				std::cout << "\t Devices :"                                                                         << std::endl;
				std::cout << "\t [V4L2 device] : V4L2 capture device (default "<< dev_name << ")"                   << std::endl;
				exit(0);
			}
		}
	}
	std::list<std::string> devList;
	while (optind<argc)
	{
		devList.push_back(argv[optind]);
		optind++;
	}
	if (devList.empty())
	{
		devList.push_back(dev_name);
	}

	// create live555 environment
	TaskScheduler* scheduler = BasicTaskScheduler::createNew();
	UsageEnvironment* env = BasicUsageEnvironment::createNew(*scheduler);
	UserAuthenticationDatabase* auth = createUserAuthenticationDatabase(userPasswordList, realm);

	// create RTSP server
	RTSPServer* rtspServer = RTSPServer::createNew(*env, rtspPort, auth, timeout);
	if (rtspServer == NULL)
	{
		std::cerr << "Failed to create RTSP server: " << env->getResultMsg() << std::endl;
	}
	else
	{
		V4l2Output* out = NULL;
		int nbSource = 0;
		std::list<std::string>::iterator devIt;
		for ( devIt=devList.begin() ; devIt!=devList.end() ; ++devIt)
		{
			std::string videoDev(*devIt);
			
			std::string baseUrl;
			if (devList.size() > 1)
			{
				baseUrl = basename(videoDev.c_str());
				baseUrl.append("/");
			}
			
			StreamReplicator* videoReplicator = NULL;
			std::string rtpFormat;
			if (!videoDev.empty())
			{
				// Init video capture
				std::cout << "Create V4L2 Source..." << videoDev << std::endl;
				
				V4L2DeviceParameters param(videoDev.c_str(), format, width, height, fps, 0);
				V4l2Capture* videoCapture = V4l2Capture::create(param, V4l2Access::IOTYPE_MMAP);
				if (videoCapture)
				{
					std::cout << "Create Source ..." << videoDev << std::endl;
					rtpFormat.assign(getRtpFormat(videoCapture->getFormat(), false));
					FramedSource* videoSource = createFramedSource(env, videoCapture->getFormat(), new DeviceCaptureAccess<V4l2Capture>(videoCapture), -1, queueSize, useThread, repeatConfig);
					if (videoSource == NULL) 
					{
						std::cerr << "Unable to create source for device " << videoDev << std::endl;
						delete videoCapture;
					}
					else
					{	
						// extend buffer size if needed
						if (videoCapture->getBufferSize() > OutPacketBuffer::maxSize)
						{
							OutPacketBuffer::maxSize = videoCapture->getBufferSize();
						}
						videoReplicator = StreamReplicator::createNew(*env, videoSource, false);
					}
				}
			}

			// Create Unicast Session
			std::list<ServerMediaSubsession*> subSession;
			if (videoReplicator)
			{
				subSession.push_back(UnicastServerMediaSubsession::createNew(*env, videoReplicator, rtpFormat));
			}
			nbSource += addSession(rtspServer, baseUrl+url, subSession);
		}

		if (nbSource>0)
		{
			// main loop
			signal(SIGINT,sighandler);
			env->taskScheduler().doEventLoop(&quit); 
			std::cout << "Exiting...." << std::endl;
		}
		
		Medium::close(rtspServer);

		if (out)
		{
			delete out;
		}
	}
	
	env->reclaim();
	delete scheduler;
	
	return 0;
}



