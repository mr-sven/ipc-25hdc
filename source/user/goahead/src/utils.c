/* vi: set sw=4 ts=4 sts=4: */
/*
 *	utils.c -- System Utilities 
 *
 *	Copyright (c) Ralink Technology Corporation All Rights Reserved.
 *
 *	$Id: utils.c,v 1.125 2012-02-03 03:37:08 chhung Exp $
 */
#include	<time.h>
#include	<signal.h>
#include	<sys/ioctl.h>
#include	<sys/time.h>

#include	<sys/socket.h>
#include	<netinet/in.h>
#include	<arpa/inet.h>

#include	"nvram.h"
#include	"ralink_gpio.h"
#include	"linux/autoconf.h"  //kernel config
#include	"config/autoconf.h" //user config

#include	"webs.h"
#include	"internet.h"
#include	"utils.h"
#include	"wireless.h"

#if defined CONFIG_USB_STORAGE && defined CONFIG_USER_STORAGE
extern void setFirmwarePath(void);
#endif

static int  getModIns(int eid, webs_t wp, int argc, char_t **argv);
static int  getCfgGeneral(int eid, webs_t wp, int argc, char_t **argv);
static int  getCfgToHTML(int eid, webs_t wp, int argc, char_t **argv);
static int  getCfgNthGeneral(int eid, webs_t wp, int argc, char_t **argv);
static int  getCfgZero(int eid, webs_t wp, int argc, char_t **argv);
static int  getCfgNthZero(int eid, webs_t wp, int argc, char_t **argv);
static int  getCfg2General(int eid, webs_t wp, int argc, char_t **argv);
static int  getCfg2NthGeneral(int eid, webs_t wp, int argc, char_t **argv);
static int  getCfg2Zero(int eid, webs_t wp, int argc, char_t **argv);
static int  getCfg2NthZero(int eid, webs_t wp, int argc, char_t **argv);
static int  getCfg3General(int eid, webs_t wp, int argc, char_t **argv);
static int  getCfg3Zero(int eid, webs_t wp, int argc, char_t **argv);
static int  getDpbSta(int eid, webs_t wp, int argc, char_t **argv);
static int  getLangBuilt(int eid, webs_t wp, int argc, char_t **argv);
static int  getMiiInicBuilt(int eid, webs_t wp, int argc, char_t **argv);
static int  getPlatform(int eid, webs_t wp, int argc, char_t **argv);
static int  getStationBuilt(int eid, webs_t wp, int argc, char_t **argv);
static int  getSysBuildTime(int eid, webs_t wp, int argc, char_t **argv);
static int  getSdkVersion(int eid, webs_t wp, int argc, char_t **argv);
static int  getSysUptime(int eid, webs_t wp, int argc, char_t **argv);
static int  getPortStatus(int eid, webs_t wp, int argc, char_t **argv);
static int  isOnePortOnly(int eid, webs_t wp, int argc, char_t **argv);
static void forceMemUpgrade(webs_t wp, char_t *path, char_t *query);
static void setOpMode(webs_t wp, char_t *path, char_t *query);
#if defined CONFIG_USB_STORAGE && defined CONFIG_USER_STORAGE
static void ScanUSBFirmware(webs_t wp, char_t *path, char_t *query);
#endif
static int  getHWNATBuilt(int eid, webs_t wp, int argc, char_t **argv);

/*********************************************************************
 * System Utilities
 */

void arplookup(char *ip, char *arp)
{
    char buf[256];
    FILE *fp = fopen("/proc/net/arp", "r");
    if(!fp){
        trace(0, T("no proc fs mounted!\n"));
        return;
    }
    strcpy(arp, "00:00:00:00:00:00");
    while(fgets(buf, 256, fp)){
        char ip_entry[32], hw_type[8],flags[8], hw_address[32];
        sscanf(buf, "%s %s %s %s", ip_entry, hw_type, flags, hw_address);
        if(!strcmp(ip, ip_entry)){
            strcpy(arp, hw_address);
            break;
        }
    }

    fclose(fp);
    return;
}


/*
 * description: kill process whose pid was recorded in file
 *              (va is supported)
 */
int doKillPid(char_t *fmt, ...)
{
	va_list		vargs;
	char_t		*pid_fname = NULL;
	struct stat	st;

	va_start(vargs, fmt);
	if (fmtValloc(&pid_fname, WEBS_BUFSIZE, fmt, vargs) >= WEBS_BUFSIZE) {
		trace(0, T("doKillPid: lost data, buffer overflow\n"));
	}
	va_end(vargs);

	if (pid_fname) {
		if (-1 == stat(pid_fname, &st)) //check the file existence
			return 0;
		doSystem("kill `cat %s`", pid_fname);
		doSystem("rm -f %s", pid_fname);
		bfree(B_L, pid_fname);
	}
	return 0;
}

/*
 * description: parse va and do system
 */
int doSystem(char_t *fmt, ...)
{
	va_list		vargs;
	char_t		*cmd = NULL;
	int			rc = 0;
	
	va_start(vargs, fmt);
	if (fmtValloc(&cmd, WEBS_BUFSIZE, fmt, vargs) >= WEBS_BUFSIZE) {
		trace(0, T("doSystem: lost data, buffer overflow\n"));
	}
	va_end(vargs);

	if (cmd) {
		trace(0, T("%s\n"), cmd);
		rc = system(cmd);
		bfree(B_L, cmd);
	}
	return rc;
}

/*
 * arguments: index - index of the Nth value
 *            values - un-parsed values
 * description: parse values delimited by semicolon, and return the value
 *              according to given index (starts from 0)
 * WARNING: the original values will be destroyed by strtok
 */
char *getNthValue(int index, char *values)
{
	int i;
	static char *tok;

	if (NULL == values)
		return NULL;
	for (i = 0, tok = strtok(values, ";");
			(i < index) && tok;
			i++, tok = strtok(NULL, ";")) {
		;
	}
	if (NULL == tok)
		return "";
	return tok;
}

/*
 * arguments: index - index of the Nth value (starts from 0)
 *            old_values - un-parsed values
 *            new_value - new value to be replaced
 * description: parse values delimited by semicolon,
 *              replace the Nth value with new_value,
 *              and return the result
 * WARNING: return the internal static string -> use it carefully
 */
char *setNthValue(int index, char *old_values, char *new_value)
{
	int i;
	char *p, *q;
	static char ret[2048];
	char buf[8][256];

	memset(ret, 0, 2048);
	for (i = 0; i < 8; i++)
		memset(buf[i], 0, 256);

	//copy original values
	for ( i = 0, p = old_values, q = strchr(p, ';')  ;
	      i < 8 && q != NULL                         ;
	      i++, p = q + 1, q = strchr(p, ';')         )
	{
		strncpy(buf[i], p, q - p);
	}
	strcpy(buf[i], p); //the last one

	//replace buf[index] with new_value
	strncpy(buf[index], new_value, 256);

	//calculate maximum index
	index = (i > index)? i : index;

	//concatenate into a single string delimited by semicolons
	strcat(ret, buf[0]);
	for (i = 1; i <= index; i++) {
		strncat(ret, ";", 2);
		strncat(ret, buf[i], 256);
	}

	return ret;
}

/*
 * arguments: values - values delimited by semicolon
 * description: parse values delimited by semicolon, and return the number of
 *              values
 */
int getValueCount(char *values)
{
	int cnt = 0;

	if (NULL == values)
		return 0;
	while (*values++ != '\0') {
		if (*values == ';')
			++cnt;
	}
	return (cnt + 1);
}

/*
 * check the existence of semicolon in str
 */
int checkSemicolon(char *str)
{
	char *c = strchr(str, ';');
	if (c)
		return 1;
	return 0;
}

/*
 * substitution of getNthValue which dosen't destroy the original value
 */
int getNthValueSafe(int index, char *value, char delimit, char *result, int len)
{
    int i=0, result_len=0;
    char *begin, *end;

    if(!value || !result || !len)
        return -1;

    begin = value;
    end = strchr(begin, delimit);

    while(i<index && end){
        begin = end+1;
        end = strchr(begin, delimit);
        i++;
    }

    //no delimit
    if(!end){
		if(i == index){
			end = begin + strlen(begin);
			result_len = (len-1) < (end-begin) ? (len-1) : (end-begin);
		}else
			return -1;
	}else
		result_len = (len-1) < (end-begin)? (len-1) : (end-begin);

	memcpy(result, begin, result_len );
	*(result+ result_len ) = '\0';

	return 0;
}

/*
 *  argument:  [IN]     index -- the index array of deleted items(begin from 0)
 *             [IN]     count -- deleted itmes count.
 *             [IN/OUT] value -- original string/return string
 *             [IN]     delimit -- delimitor
 */
int deleteNthValueMulti(int index[], int count, char *value, char delimit)
{
	char *begin, *end;
	int i=0,j=0;
	int need_check_flag=0;
	char *buf = strdup(value);

	begin = buf;

	end = strchr(begin, delimit);
	while(end){
		if(i == index[j]){
			memset(begin, 0, end - begin );
			if(index[j] == 0)
				need_check_flag = 1;
			j++;
			if(j >=count)
				break;
		}
		begin = end;

		end = strchr(begin+1, delimit);
		i++;
	}

	if(!end && index[j] == i)
		memset(begin, 0, strlen(begin));

	if(need_check_flag){
		for(i=0; i<strlen(value); i++){
			if(buf[i] == '\0')
				continue;
			if(buf[i] == ';')
				buf[i] = '\0';
			break;
		}
	}

	for(i=0, j=0; i<strlen(value); i++){
		if(buf[i] != '\0'){
			value[j++] = buf[i];
		}
	}
	value[j] = '\0';

	free(buf);
	return 0;
}



/*
 * nanosleep(2) don't depend on signal SIGALRM and could cooperate with
 * other SIGALRM-related functions(ex. setitimer(2))
 */
unsigned int Sleep(unsigned int secs)
{
	int rc;
	struct timespec ts, remain;
	ts.tv_sec  = secs;
	ts.tv_nsec = 0;

sleep_again:
	rc = nanosleep(&ts, &remain);
	if(rc == -1 && errno == EINTR){
		ts.tv_sec = remain.tv_sec;
		ts.tv_nsec = remain.tv_nsec;
		goto sleep_again;
	}	
	return 0;
}

/*
 * The setitimer() is Linux-specified.
 */
int setTimer(int microsec, void ((*sigroutine)(int)))
{
	struct itimerval value, ovalue;
   
	signal(SIGALRM, sigroutine);
	value.it_value.tv_sec = 0;
	value.it_value.tv_usec = microsec;
	value.it_interval.tv_sec = 0;
	value.it_interval.tv_usec = microsec;
	return setitimer(ITIMER_REAL, &value, &ovalue);
}

void stopTimer(void)
{
	struct itimerval value, ovalue;
   
	value.it_value.tv_sec = 0;
	value.it_value.tv_usec = 0;
	value.it_interval.tv_sec = 0;
	value.it_interval.tv_usec = 0;
	setitimer(ITIMER_REAL, &value, &ovalue);
}

/*
 * configure LED blinking with proper frequency (privatly use only)
 *   on: number of ticks that LED is on
 *   off: number of ticks that LED is off
 *   blinks: number of on/off cycles that LED blinks
 *   rests: number of on/off cycles that LED resting
 *   times: stop blinking after <times> times of blinking
 * where 1 tick == 100 ms
 */
static int gpioLedSet(int gpio, unsigned int on, unsigned int off,
		unsigned int blinks, unsigned int rests, unsigned int times)
{
	int fd;
	ralink_gpio_led_info led;

	//parameters range check
	if (gpio < 0 || gpio >= RALINK_GPIO_NUMBER ||
			on > RALINK_GPIO_LED_INFINITY ||
			off > RALINK_GPIO_LED_INFINITY ||
			blinks > RALINK_GPIO_LED_INFINITY ||
			rests > RALINK_GPIO_LED_INFINITY ||
			times > RALINK_GPIO_LED_INFINITY) {
		return -1;
	}
	led.gpio = gpio;
	led.on = on;
	led.off = off;
	led.blinks = blinks;
	led.rests = rests;
	led.times = times;

	fd = open("/dev/gpio", O_RDONLY);
	if (fd < 0) {
		perror("/dev/gpio");
		return -1;
	}
	if (ioctl(fd, RALINK_GPIO_LED_SET, &led) < 0) {
		perror("ioctl");
		close(fd);
		return -1;
	}
	close(fd);
	return 0;
}

int ledAlways(int gpio, int on)
{
	if (on)
		return gpioLedSet(gpio, RALINK_GPIO_LED_INFINITY, 0, 1, 1, RALINK_GPIO_LED_INFINITY);
	else
		return gpioLedSet(gpio, 0, RALINK_GPIO_LED_INFINITY, 1, 1, RALINK_GPIO_LED_INFINITY);
}

/*
 * concatenate a string with an integer
 * ex: racat("SSID", 1) will return "SSID1"
 */
char *racat(char *s, int i)
{
	static char str[32];
	snprintf(str, 32, "%s%1d", s, i);
	return str;
}

void websLongWrite(webs_t wp, char *longstr)
{
    char tmp[513] = {0};
    int len = strlen(longstr);
    char *end = longstr + len;

    while(longstr < end){
        strncpy(tmp, longstr, 512);
        websWrite(wp, T("%s"), tmp);
        longstr += 512;
    }
    return;
}

/* 
 * arguments: type - 0 = return the status of module insertion (default)
 *                   1 = write the status of module insertion
 *            modname - module name
 * description: test the insertion of module through /proc/modules
 * result: -1 = fopen error, 1 = module inserted, 0 = not inserted yet
 */
static int getModIns(int eid, webs_t wp, int argc, char_t **argv)
{
	int i, type, result = 0;
	char_t *modname;
	FILE *fp;
	char buf[128];

	if (ejArgs(argc, argv, T("%d %s"), &type, &modname) < 2) {
		return websWrite(wp, T("Insufficient args\n"));
	}

	if (NULL == (fp = fopen("/proc/modules", "r"))) {
		//error(E_L, E_LOG, T("getModIns: open /proc/modules error"));
		if (1 == type)
			return websWrite(wp, T("-1"));
		else {
			ejSetResult(eid, "-1");
			return 0;
		}
	}

	while (NULL != fgets(buf, 128, fp)) {
		i = 0;
		while (!isspace(buf[i++]))
			;
		buf[i - 1] = '\0';
		if (!strcmp(buf, modname)) {
			result = 1;
			break;
		}
	}
	fclose(fp);

	if (1 == type)
		return websWrite(wp, T("%d"), result);
	ejSetResult(eid, result);
	return 0;
}

/* 
 * arguments: type - 0 = return the configuration of 'field' (default)
 *                   1 = write the configuration of 'field' 
 *            field - parameter name in nvram
 * description: read general configurations from nvram
 *              if value is NULL -> ""
 */
static int getCfgGeneral(int eid, webs_t wp, int argc, char_t **argv)
{
	int type;
	char_t *field;
	char *value;

	if (ejArgs(argc, argv, T("%d %s"), &type, &field) < 2) {
		return websWrite(wp, T("Insufficient args\n"));
	}
	value = (char *) nvram_bufget(RT2860_NVRAM, field);
	if (1 == type) {
		if (NULL == value)
			return websWrite(wp, T(""));
		return websWrite(wp, T("%s"), value);
	}
	if (NULL == value)
		ejSetResult(eid, "");
	ejSetResult(eid, value);
	return 0;
}

static int getCfgToHTML(int eid, webs_t wp, int argc, char_t **argv)
{
	int type; 
	unsigned int offset, len;
	char_t *field;
	char *value, *head, *tok;
	char ret[32+1];

	if (ejArgs(argc, argv, T("%d %s"), &type, &field) < 2) {
		return websWrite(wp, T("Insufficient args\n"));
	}
	value = (char *) nvram_bufget(RT2860_NVRAM, field);
	memset(ret, 0, 33);
	tok = value;
	head = tok;
	do {
		if (*tok == 34) {
			offset = (unsigned int) head - (unsigned int) value;
			len = (unsigned int) tok - (unsigned int) head;
			strncat(ret, value+offset, len);
			strcat(ret, "&#34;");
			head = tok+1;
		} 
		else if (*tok == 39) {
			offset = head - (unsigned int) value;
			len = (unsigned int) tok - (unsigned int) head;
			strncat(ret, value+offset, len);
			strcat(ret, "&#39;");
			head = tok+1;
		}
		tok++;
	} while (*tok != '\0');
	offset = (unsigned int) head - (unsigned int) value;
	strcat(ret, value+offset);
	if (1 == type) {
		if (NULL == ret)
			return websWrite(wp, T(""));
		return websWrite(wp, T("%s"), ret);
	}
	if (NULL == ret)
		ejSetResult(eid, "");
	ejSetResult(eid, ret);
	return 0;
}
/* 
 * arguments: type - 0 = return the configuration of 'field' (default)
 *                   1 = write the configuration of 'field' 
 *            field - parameter name in nvram
 *            idx - index of nth
 * description: read general configurations from nvram (if value is NULL -> "")
 *              parse it and return the Nth value delimited by semicolon
 */
static int getCfgNthGeneral(int eid, webs_t wp, int argc, char_t **argv)
{
	int type, idx;
	char_t *field;
	char *value;
	char nth[128];

	memset(nth, 0, 128);
	if (ejArgs(argc, argv, T("%d %s %d"), &type, &field, &idx) < 3) {
		return websWrite(wp, T("Insufficient args\n"));
	}
	value = (char *) nvram_bufget(RT2860_NVRAM, field);
	if (1 == type) {
		if (NULL == value)
			return websWrite(wp, T(""));
		getNthValueSafe(idx, value, ';', nth, 128);
		if (NULL == nth)
			return websWrite(wp, T(""));
		return websWrite(wp, T("%s"), nth);
	}
	if (NULL == value)
		ejSetResult(eid, "");
	getNthValueSafe(idx, value, ';', nth, 128);
	if (NULL == nth)
		ejSetResult(eid, "");
	ejSetResult(eid, value);
	return 0;
}

/*
 * arguments: type - 0 = return the configuration of 'field' (default)
 *                   1 = write the configuration of 'field' 
 *            field - parameter name in nvram
 * description: read general configurations from nvram
 *              if value is NULL -> "0"
 */
static int getCfgZero(int eid, webs_t wp, int argc, char_t **argv)
{
	int type;
	char_t *field;
	char *value;

	if (ejArgs(argc, argv, T("%d %s"), &type, &field) < 2) {
		return websWrite(wp, T("Insufficient args\n"));
	}
	value = (char *) nvram_bufget(RT2860_NVRAM, field);
	if (1 == type) {
		if (!strcmp(value, ""))
			return websWrite(wp, T("0"));
		return websWrite(wp, T("%s"), value);
	}
	if (!strcmp(value, ""))
		ejSetResult(eid, "0");
	ejSetResult(eid, value);
	return 0;
}

/* 
 * arguments: type - 0 = return the configuration of 'field' (default)
 *                   1 = write the configuration of 'field' 
 *            field - parameter name in nvram
 *            idx - index of nth
 * description: read general configurations from nvram (if value is NULL -> "0")
 *              parse it and return the Nth value delimited by semicolon
 */
static int getCfgNthZero(int eid, webs_t wp, int argc, char_t **argv)
{
	int type, idx;
	char_t *field;
	char *value;
	char nth[128];

	memset(nth, 0, 128);
	if (ejArgs(argc, argv, T("%d %s %d"), &type, &field, &idx) < 3) {
		return websWrite(wp, T("Insufficient args\n"));
	}
	value = (char *) nvram_bufget(RT2860_NVRAM, field);
	if (1 == type) {
		if (!strcmp(value, ""))
			return websWrite(wp, T("0"));
		getNthValueSafe(idx, value, ';', nth, 128);
		if (NULL == nth)
			return websWrite(wp, T("0"));
		return websWrite(wp, T("%s"), nth);
	}
	if (!strcmp(value, ""))
		ejSetResult(eid, "0");
	getNthValueSafe(idx, value, ';', nth, 128);
	if (NULL == nth)
		ejSetResult(eid, "0");
	ejSetResult(eid, value);
	return 0;
}

/* 
 * arguments: type - 0 = return the configuration of 'field' (default)
 *                   1 = write the configuration of 'field' 
 *            field - parameter name in nvram
 * description: read general configurations from nvram
 *              if value is NULL -> ""
 */
static int  getCfg2General(int eid, webs_t wp, int argc, char_t **argv)
{
	return 0;
}

static int  getCfg2NthGeneral(int eid, webs_t wp, int argc, char_t **argv)
{
	return 0;
}

static int  getCfg2Zero(int eid, webs_t wp, int argc, char_t **argv)
{
	return 0;
}

static int  getCfg2NthZero(int eid, webs_t wp, int argc, char_t **argv)
{
	return 0;
}

static int getDpbSta(int eid, webs_t wp, int argc, char_t **argv)
{
	return websWrite(wp, T("0"));
}

static int getLangBuilt(int eid, webs_t wp, int argc, char_t **argv)
{
	char_t *lang;

	if (ejArgs(argc, argv, T("%s"), &lang) < 1) {
		return websWrite(wp, T("0"));
	}
	if (!strncmp(lang, "en", 3))
#ifdef CONFIG_USER_GOAHEAD_LANG_EN
		return websWrite(wp, T("1"));
#else
		return websWrite(wp, T("0"));
#endif
	else if (!strncmp(lang, "zhtw", 5))
#ifdef CONFIG_USER_GOAHEAD_LANG_ZHTW
		return websWrite(wp, T("1"));
#else
		return websWrite(wp, T("0"));
#endif
	else if (!strncmp(lang, "zhcn", 5))
#ifdef CONFIG_USER_GOAHEAD_LANG_ZHCN
		return websWrite(wp, T("1"));
#else
		return websWrite(wp, T("0"));
#endif

	return websWrite(wp, T("0"));
}

static int getMiiInicBuilt(int eid, webs_t wp, int argc, char_t **argv)
{
#if defined (CONFIG_RTDEV_MII)
	return websWrite(wp, T("1"));
#else
	return websWrite(wp, T("0"));
#endif
}

/*
 * description: write the current system platform accordingly
 */
static int getPlatform(int eid, webs_t wp, int argc, char_t **argv)
{
	char platform[8];
	strcpy(platform, "RT5350");
	return websWrite(wp, T("%s webcam"), platform);
}

static int getHWNATBuilt(int eid, webs_t wp, int argc, char_t **argv)
{
	return websWrite(wp, T("0"));
}

static int getStationBuilt(int eid, webs_t wp, int argc, char_t **argv)
{
	return websWrite(wp, T("1"));
}

/*
 * description: write System build time
 */
static int getSysBuildTime(int eid, webs_t wp, int argc, char_t **argv)
{
	return websWrite(wp, T("%s"), __DATE__);
}

/*
 * description: write RT288x SDK version
 */
static int getSdkVersion(int eid, webs_t wp, int argc, char_t **argv)
{
	FILE *fp = fopen("/etc_ro/web/cgi-bin/History", "r");
	char version[16] = "";

	if (fp != NULL)
	{
		char buf[30];
		while (fgets(buf, sizeof(buf), fp) != NULL) {
			if (strncasecmp(buf, "Version", 7) != 0)
				continue;
			sscanf(buf, "%*s%s", version);
			break;
		}
		fclose(fp);
	}

	return websWrite(wp, T("%s"), version);
}

/*
 * description: write System Uptime
 */
static int getSysUptime(int eid, webs_t wp, int argc, char_t **argv)
{
	struct tm *utime;
	time_t usecs;

	time(&usecs);
	utime = localtime(&usecs);

	if (utime->tm_hour > 0)
		return websWrite(wp, T("%d hour%s, %d min%s, %d sec%s"),
				utime->tm_hour, (utime->tm_hour == 1)? "" : "s",
				utime->tm_min, (utime->tm_min == 1)? "" : "s",
				utime->tm_sec, (utime->tm_sec == 1)? "" : "s");
	else if (utime->tm_min > 0)
		return websWrite(wp, T("%d min%s, %d sec%s"),
				utime->tm_min, (utime->tm_min == 1)? "" : "s",
				utime->tm_sec, (utime->tm_sec == 1)? "" : "s");
	else
		return websWrite(wp, T("%d sec%s"),
				utime->tm_sec, (utime->tm_sec == 1)? "" : "s");
	return 0;
}

static int getPortStatus(int eid, webs_t wp, int argc, char_t **argv)
{
	websWrite(wp, T("-1"));
	return 0;
}

inline int getOnePortOnly(void)
{
	return 0;
}

static int isOnePortOnly(int eid, webs_t wp, int argc, char_t **argv)
{
	if( getOnePortOnly() == 1)
		websWrite(wp, T("true"));
	else
		websWrite(wp, T("false"));		 
	return 0;
}

void redirect_wholepage(webs_t wp, const char *url)
{
	websWrite(wp, T("HTTP/1.1 200 OK\nContent-type: text/html\nPragma: no-cache\nCache-Control: no-cache\n\n"));
	websWrite(wp, T("<html><head><script language=\"JavaScript\">"));
	websWrite(wp, T("parent.location.replace(\"%s\");"), url);
	websWrite(wp, T("</script></head></html>"));
}

int netmask_aton(const char *ip)
{
	int i, a[4], result = 0;
	sscanf(ip, "%d.%d.%d.%d", &a[0], &a[1], &a[2], &a[3]);
	for(i=0; i<4; i++){	//this is dirty
		if(a[i] == 255){
			result += 8;
			continue;
		}
		if(a[i] == 254)
			result += 7;
		if(a[i] == 252)
			result += 6;
		if(a[i] == 248)
			result += 5;
		if(a[i] == 240)
			result += 4;
		if(a[i] == 224)
			result += 3;
		if(a[i] == 192)
			result += 2;
		if(a[i] == 128)
			result += 1;
		//if(a[i] == 0)
		//	result += 0;
		break;
	}
	return result;
}
static void forceMemUpgrade(webs_t wp, char_t *path, char_t *query)
{
	char_t *mode  = websGetVar(wp, T("ForceMemUpgradeSelect"), T("0"));
	if(!mode)
		return;
	if(!strcmp(mode, "1"))
		nvram_bufset(RT2860_NVRAM, "Force_mem_upgrade", "1");
	else
		nvram_bufset(RT2860_NVRAM, "Force_mem_upgrade", "0");
	nvram_commit(RT2860_NVRAM);
    websHeader(wp);
    websWrite(wp, T("<h2>force mem upgrade</h2>\n"));
    websWrite(wp, T("mode: %s<br>\n"), mode);
    websFooter(wp);
    websDone(wp, 200);	
}

/* goform/setOpMode */
static void setOpMode(webs_t wp, char_t *path, char_t *query)
{
}

void ConverterStringToDisplay(char *str)
{
    int  len, i;
    char buffer[193];
    char *pOut;

    memset(buffer,0,193);
    len = strlen(str);
    pOut = &buffer[0];

    for (i = 0; i < len; i++) {
		switch (str[i]) {
			case 38:
				strcpy (pOut, "&amp;");
				pOut += 5;
				break;

			case 60: 
				strcpy (pOut, "&lt;");
				pOut += 4;
				break;

			case 62: 
				strcpy (pOut, "&gt;");
				pOut += 4;
				break;

			case 34:
				strcpy (pOut, "&#34;");
				pOut += 5;
				break;

			case 39:
				strcpy (pOut, "&#39;");
				pOut += 5;
				break;
			case 32:
				strcpy (pOut, "&nbsp;");
				pOut += 6;
				break;

			default:
				if ((str[i]>=0) && (str[i]<=31)) {
					//Device Control Characters
					sprintf(pOut, "&#%02d;", str[i]);
					pOut += 5;
				} else if ((str[i]==39) || (str[i]==47) || (str[i]==59) || (str[i]==92)) {
					// ' / ; (backslash)
					sprintf(pOut, "&#%02d;", str[i]);
					pOut += 5;
				} else if (str[i]>=127) {
					//Device Control Characters
					sprintf(pOut, "&#%03d;", str[i]);
					pOut += 6;
				} else {
					*pOut = str[i];
					pOut++;
				}
				break;
		}
    }
    *pOut = '\0';
    strcpy(str, buffer);
}

