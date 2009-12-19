/*
 * Copyright 2004, ASUSTek Inc.
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND ASUS GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id$
 */

#define DNSMASQ
#define DDNSCONF
#define DDNSDAEMON

#ifdef ASUS_EXT
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/fcntl.h>
#include <dirent.h>
#include <sys/mount.h>
#include <bcmnvram.h>
#include <netconf.h>
#include <shutils.h>
#include <rc.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <ctype.h>
#if defined(__UCLIBC__)
#include <crypt.h>
#endif
#include <mntent.h>
#include "iboxcom.h"
#include "lp.h"

#define logs(s) syslog(LOG_NOTICE, s)

#ifdef USB_SUPPORT
#define WEBCAM_SUPPORT 1
#define PRINTER_SUPPORT 1
#define MASSSTORAGE_SUPPORT 1
#define AUDIO_SUPPORT 1
#endif

//#define PARPORT_SUPPORT 
//#define USBCOPY_SUPPORT 1

enum
{
	WEB_NONE = 0,
	WEB_PWCWEB,
	WEB_OVWEB,
	WEB_AUDIO
} WEBTYPE;

char *PWCLIST[] = {"471","69a","46d","55d","41e","4cc","d81", NULL};
char *OVLIST[] = {"5a9","813","b62", NULL};
char buf_g[512];

int remove_usb_audio(char *product);
static int umount_all_part(char *product, int scsi_host_no);
static struct mntent *findmntent(char *file);


void diag_PaN(void)
{
	FILE *fp;
        
	/* dump pci device */
	fp=fopen("/proc/pci", "r");
	if (fp!=NULL)
	{
		while(fgets(buf_g, sizeof(buf_g), fp))
		{
			if(strstr(buf_g, "PCI device"))
				fprintf(stderr, buf_g);
		}
		fclose(fp);
	}


#ifdef RT2400_SUPPORT
	if (nvram_match("nobr", "1"))
		fprintf(stderr, "dumping 0x4301 for manu.\n");
#endif

#ifdef USB_SUPPORT
	if (nvram_match("usb_device", "1"))
	{
		fprintf(stderr, "new USB device\r\n");
	}
#endif

#ifdef PRINTER_SUPPORT
#ifdef PARPORT_SUPPORT	
	char *token;
    	char                                mfr[32];
    	char                                model[64];
    	int                                 fd;
    	struct parport_splink_device_info   prn_info;

    if( (fd=open("/dev/lp0", O_RDWR)) < 0 ) //Someone is opening the lp0
    {
        fp=fopen("/proc/sys/dev/parport/parport0/devices/lp/deviceid","r");

        if( fp != NULL)
        {    
            while ( fgets(buf_g, sizeof(buf_g), fp) != NULL )  
            {
                if(buf_g[0] == '\n')
                {
                    continue;
                }
                if(strncmp(buf_g , "Model: " , strlen("Model: ")) == 0)
                {
                    token = buf_g + strlen("Model: ");
		    sprintf(buf_g, "PARPORT: %s", token);
		    fprintf(stderr, "%s\r\n", buf_g);
		    //logmessage("PARPORT", buf_g);
                }                
	    }	
	    fclose(fp);
	}
	//else logmessage("NO", "No printer found");
    }
    else
    {
        if( ioctl(fd, LPGETID, &prn_info) <0 )
        {
	    //logmessage("PaN", "No printer found");
            //PRINT("ioctl error\n");
        }
        else
        {
            memccpy(mfr , prn_info.mfr , 1 , 32);
            memccpy(model , prn_info.model , 1 , 32);
        
            sprintf(buf_g, "PARPORT: %s", model);
	    fprintf(stderr, "%s\r\n", buf_g);
            //logmessage("PARPORT", buf_g);
        }
	close(fd);
   }
#endif
#endif
   fprintf(stderr, "echo for PaN ::: &&&PaN\r\n");
}

#if defined(DNSMASQ) || defined(DDNSCONF) || 1
size_t
fappend(char *name, FILE *f)
{
	size_t size = 0, count;
	
	FILE *fp = fopen(name, "r");

	if (fp != NULL) {
		char buf[4096];
		
		while ((count = fread(buf, 1, sizeof(buf), fp)) > 0)
			size += fwrite(buf, 1, count, f);
	
		fclose(fp);
	}
	
	return size;
}
#endif

#ifndef DNSMASQ
int
start_dhcpd(void)
{
	FILE *fp;
	char *dhcpd_argv[] = {"udhcpd", "/tmp/udhcpd.conf", NULL, NULL};
	char *slease = "/tmp/udhcpd-br0.sleases";
	pid_t pid;

	if (nvram_match("router_disable", "1") || nvram_invmatch("lan_proto", "dhcp"))
		return 0;

	dprintf("%s %s %s %s\n",
		nvram_safe_get("lan_ifname"),
		nvram_safe_get("dhcp_start"),
		nvram_safe_get("dhcp_end"),
		nvram_safe_get("lan_lease"));

	if (!(fp = fopen("/tmp/udhcpd-br0.leases", "a"))) {
		perror("/tmp/udhcpd-br0.leases");
		return errno;
	}
	fclose(fp);

	/* Write configuration file based on current information */
	if (!(fp = fopen("/tmp/udhcpd.conf", "w"))) {
		perror("/tmp/udhcpd.conf");
		return errno;
	}
	
	fprintf(fp, "pidfile /var/run/udhcpd-br0.pid\n");
	fprintf(fp, "start %s\n", nvram_safe_get("dhcp_start"));
	fprintf(fp, "end %s\n", nvram_safe_get("dhcp_end"));
	fprintf(fp, "interface %s\n", nvram_safe_get("lan_ifname"));
	fprintf(fp, "remaining yes\n");
	fprintf(fp, "lease_file /tmp/udhcpd-br0.leases\n");
	fprintf(fp, "option subnet %s\n", nvram_safe_get("lan_netmask"));
	
	if (nvram_invmatch("dhcp_gateway_x",""))
	    fprintf(fp, "option router %s\n", nvram_safe_get("dhcp_gateway_x"));	else	
	    fprintf(fp, "option router %s\n", nvram_safe_get("lan_ipaddr"));	
	
	if (nvram_invmatch("dhcp_dns1_x",""))		
		fprintf(fp, "option dns %s\n", nvram_safe_get("dhcp_dns1_x"));		
	fprintf(fp, "option dns %s\n", nvram_safe_get("lan_ipaddr"));
	fprintf(fp, "option lease %s\n", nvram_safe_get("dhcp_lease"));

	if (nvram_invmatch("dhcp_wins_x",""))		
		fprintf(fp, "option wins %s\n", nvram_safe_get("dhcp_wins_x"));		
	if (nvram_invmatch("lan_domain", ""))
		fprintf(fp, "option domain %s\n", nvram_safe_get("lan_domain"));
	fclose(fp);

	
	if (nvram_match("dhcp_static_x","1"))
	{	
		write_static_leases(slease);
		dhcpd_argv[2] = slease;
	}
	else
	{
		dhcpd_argv[2] = NULL;
	}

	_eval(dhcpd_argv, NULL, 0, &pid);


	dprintf("done\n");
	return 0;
}

int
stop_dhcpd(void)
{
	char sigusr1[] = "-XX";
	int ret;

/*
* Process udhcpd handles two signals - SIGTERM and SIGUSR1
*
*  - SIGUSR1 saves all leases in /tmp/udhcpd.leases
*  - SIGTERM causes the process to be killed
*
* The SIGUSR1+SIGTERM behavior is what we like so that all current client
* leases will be honorred when the dhcpd restarts and all clients can extend
* their leases and continue their current IP addresses. Otherwise clients
* would get NAK'd when they try to extend/rebind their leases and they 
* would have to release current IP and to request a new one which causes 
* a no-IP gap in between.
*/
	ret = eval("killall", "udhcpd");

	dprintf("done\n");
	return ret;
}

int
start_dns(void)
{
	FILE *fp;
	char *dproxy_argv[] = {"dproxy", "-d", "-c", "/tmp/dproxy.conf", NULL};
	pid_t pid;
	char word[100], *tmp;
	int ret, active;

	if (nvram_match("router_disable", "1"))
		return 0;


	/* Create resolv.conf with empty nameserver list */
	if (!(fp = fopen("/tmp/resolv.conf", "r")))
	{
		if (!(fp = fopen("/tmp/resolv.conf", "w"))) 
		{
			perror("/tmp/resolv.conf");
			return errno;
		}
		else fclose(fp);
	}
	else fclose(fp);

	if (!(fp = fopen("/tmp/dproxy.conf", "w"))) {
		perror("/tmp/dproxy.conf");
		return errno;
	}

	fprintf(fp, "name_server=140.113.1.1\n");
	fprintf(fp, "ppp_detect=no\n");
	fprintf(fp, "purge_time=1200\n");
	fprintf(fp, "deny_file=/tmp/dproxy.deny\n");
	fprintf(fp, "cache_file=/tmp/dproxy.cache\n");
	fprintf(fp, "hosts_file=/tmp/hosts\n");
	fprintf(fp, "dhcp_lease_file=\n");
	fprintf(fp, "ppp_dev=/var/run/ppp0.pid\n");
	fclose(fp);

	// if user want to set dns server by himself
	if (nvram_invmatch("wan_dnsenable_x", "1"))	
	{
		/* Write resolv.conf with upstream nameservers */
		if (!(fp = fopen("/tmp/resolv.conf", "w"))) {
			perror("/tmp/resolv.conf");
			return errno;
		}
	
		if (nvram_invmatch("wan_dns1_x",""))
			fprintf(fp, "nameserver %s\n", nvram_safe_get("wan_dns1_x"));		
		if (nvram_invmatch("wan_dns2_x",""))
			fprintf(fp, "nameserver %s\n", nvram_safe_get("wan_dns2_x"));
		fclose(fp);
	}

	active = timecheck_item(nvram_safe_get("url_date_x"), 
				nvram_safe_get("url_time_x"));

	if (nvram_match("url_enable_x", "1") && active) 
	{
		int i;
		
		if (!(fp = fopen("/tmp/dproxy.deny", "w"))) {
			perror("/tmp/dproxy.deny");
			return errno;
		}

		for(i=0; i<atoi(nvram_safe_get("url_num_x")); i++) {
			sprintf(word, "url_keyword_x%d", i);
			fprintf(fp, "%s\n", nvram_safe_get(word));
		}
	
		fclose(fp);	
	}
	else unlink("/tmp/dproxy.deny");


	if (!(fp = fopen("/tmp/hosts", "w"))) {
		perror("/tmp/hosts");
		return errno;
	}

	fprintf(fp, "127.0.0.1 localhost.localdomain localhost\n");
	fprintf(fp, "%s	my.router\n", nvram_safe_get("lan_ipaddr"));
	fprintf(fp, "%s	my.%s\n", nvram_safe_get("lan_ipaddr"), nvram_safe_get("productid"));
	
	if (nvram_invmatch("lan_hostname", ""))
	{
		fprintf(fp, "%s %s.%s %s\n", nvram_safe_get("lan_ipaddr"),
					nvram_safe_get("lan_hostname"),
					nvram_safe_get("lan_domain"),
					nvram_safe_get("lan_hostname"));
	}	
	fclose(fp);	
		
	_eval(dproxy_argv, NULL, 0, &pid);

	dprintf("done\n");
	return ret;
}	

int
stop_dns(void)
{
	int ret = eval("killall", "dproxy");
	unlink("/tmp/dproxy.deny");		
	dprintf("done\n");
	return ret;
}

#else

int
start_dns(void)
{
	FILE *fp;
	char *argv[] = {"dnsmasq", NULL};
	pid_t pid;

	size_t ethers = 0;
	
	int ret;

	/* Create resolv.conf with empty nameserver list */
	if (!(fp = fopen("/tmp/resolv.conf", "a"))) {
		perror("/tmp/resolv.conf");
		return errno;
	}

	// if user want to set dns server by himself
	if (nvram_invmatch("wan_dnsenable_x", "1"))	
	{
		/* Write resolv.conf with upstream nameservers */
		if (nvram_invmatch("wan_dns1_x",""))
			fprintf(fp, "nameserver %s\n", nvram_safe_get("wan_dns1_x"));		
		if (nvram_invmatch("wan_dns2_x",""))
			fprintf(fp, "nameserver %s\n", nvram_safe_get("wan_dns2_x"));
	}

	fclose(fp);

	/* create /etc/hosts */
	if (!(fp = fopen("/etc/hosts", "a"))) {
		perror("/etc/hosts");
		return errno;
	}

	fprintf(fp, "127.0.0.1 localhost.localdomain localhost\n");
	fprintf(fp, "%s %s my.router my.%s\n", nvram_safe_get("lan_ipaddr"),
			nvram_safe_get("lan_hostname"), nvram_safe_get("productid"));

	if (nvram_match("dhcp_static_x","1"))
	{	
		int i;
		char ip[32], name[32];
		for (i = 0; i < atoi(nvram_safe_get("dhcp_staticnum_x")); i++) 
		{
			sprintf(ip, "dhcp_staticip_x%d", i);
			sprintf(name, "dhcp_staticname_x%d", i);

			if (inet_addr_(nvram_safe_get(ip)) != INADDR_ANY && 
				nvram_invmatch(name, "")) 
			{
				fprintf(fp, "%s %s\n", nvram_get(ip), nvram_get(name));
			}
		}
	}
	
	fappend("/usr/local/etc/hosts", fp);
	fclose(fp);

	if (nvram_match("router_disable", "1"))
		return 0;

	/* create /etc/ethers */
	if (!(fp = fopen("/etc/ethers", "w"))) {
		perror("/etc/ethers");
	} else {
		if (nvram_match("dhcp_static_x","1"))
		{	
			int i;
			char buf[32], *mac;
			for (i = 0; i < atoi(nvram_safe_get("dhcp_staticnum_x")); i++) 
			{
				sprintf(buf, "dhcp_staticmac_x%d", i);
				
				if (strlen(mac = nvram_safe_get(buf)) == 12) {
					sprintf(buf, "dhcp_staticip_x%d", i);
					ethers += fprintf(fp, "%c%c:%c%c:%c%c:%c%c:%c%c:%c%c %s\n", 
						mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], 
						mac[6], mac[7], mac[8], mac[9], mac[10], mac[11],
						nvram_safe_get(buf));
				}
				
			}
		}
	
		ethers += fappend("/usr/local/etc/ethers", fp);
		fclose(fp);
	}

	/* start playing with conf file */
	if (!(fp = fopen("/etc/dnsmasq.conf", "w"))) {
		perror("/etc/dnsmasq.conf");
		return errno;
	}

	fprintf(fp, "user=nobody\n"
		    "resolv-file=/tmp/resolv.conf\nno-poll\n"
		    "interface=%s\n", nvram_safe_get("lan_ifname"));

	if (nvram_invmatch("lan_domain", "")) {
		fprintf(fp, "domain=%s\n"
			    "expand-hosts\n", nvram_get("lan_domain"));
	}
	
	fprintf(fp, "no-negcache\n"
		    "cache-size=512\n");
	
	if (nvram_match("lan_proto", "dhcp")) {
		fprintf(fp, "dhcp-leasefile=/tmp/dnsmasq.log\n");
		fprintf(fp, "dhcp-range=lan,%s,%s,%s\n", nvram_safe_get("dhcp_start"), 
			nvram_safe_get("dhcp_end"), nvram_safe_get("dhcp_lease"));

		if (nvram_invmatch("dhcp_dns1_x",""))
			fprintf(fp, "dhcp-option=lan,6,%s,0.0.0.0\n", nvram_safe_get("dhcp_dns1_x"));
		if (nvram_invmatch("dhcp_wins_x",""))		
			fprintf(fp, "dhcp-option=lan,44,%s\n"
				    "dhcp-option=lan,46,8\n", nvram_safe_get("dhcp_wins_x"));
		if (nvram_invmatch("lan_domain", ""))
			fprintf(fp, "dhcp-option=lan,15,%s\n", nvram_safe_get("lan_domain"));
		if (nvram_invmatch("dhcp_gateway_x",""))
			fprintf(fp, "dhcp-option=lan,3,%s\n", nvram_safe_get("dhcp_gateway_x"));
			
		if (ethers)
			fprintf(fp, "read-ethers\n");
			
		fprintf(fp, "dhcp-authoritative\n");
	}
	
	fappend("/usr/local/etc/dnsmasq.conf", fp);
	fclose(fp);

	/* launch it */		
	ret = _eval(argv, NULL, 0, &pid);

	dprintf("done\n");
	return ret;
}	

int
stop_dns(void)
{
	int ret = eval("killall", "dnsmasq");

	dprintf("done\n");
	return ret;
}

int start_dhcpd(void)
{
	return 0;
}

int stop_dhcpd(void)
{
	return 0;
}

#endif // DNSMASQ

int
ddns_updated_main()
{
	FILE *fp;
	char buf[64], *ip;

	if (!(fp=fopen("/tmp/ddns.cache", "r"))) return 0;
	
	fgets(buf, sizeof(buf), fp);
	fclose(fp);

	if (!(ip=strchr(buf, ','))) return 0;
	
	nvram_set("ddns_cache", buf);
	nvram_set("ddns_ipaddr", ip+1);
	nvram_set("ddns_status", "1");
	nvram_commit();

	logmessage("ddns", "ddns update ok");

	dprintf("done\n");

	return 0;
}
	

int 
start_ddns(void)
{
	FILE *fp;
//	char buf[64];
	char *wan_ip, *ddns_cache;
	char server[32];
	char user[32];
	char passwd[32];
	char host[64];
	char service[32];
	char usrstr[64];
	char wan_ifname[16];
	int  wild=nvram_match("ddns_wildcard_x", "1");


	if (nvram_match("router_disable", "1")) return -1;
	
	if (nvram_invmatch("ddns_enable_x", "1")) return -1;
	
	if ((wan_ip = nvram_safe_get("wan_ipaddr_t"))==NULL) return -1;

	if (nvram_match("ddns_ipaddr", wan_ip) ||
	   (inet_addr(wan_ip)==inet_addr(nvram_safe_get("ddns_ipaddr"))))
	{
		logmessage("ddns", "IP address has not changed since the last update");
		return 0;
	}

	// TODO : Check /tmp/ddns.cache to see current IP in DDNS
	// update when,
	// 	1. if ipaddr!= ipaddr in cache
	// 	
        // update
	// * nvram ddns_cache, the same with /tmp/ddns.cache


	if ((fp=fopen("/tmp/ddns.cache", "r"))==NULL && 
	     (ddns_cache=nvram_get("ddns_cache"))!=NULL)
	{
		if ((fp = fopen("/tmp/ddns.cache", "w+"))!=NULL)
		{
			fprintf(fp, "%s", ddns_cache);
			fclose(fp);
		}
	}

	strcpy(server, nvram_safe_get("ddns_server_x"));
	strcpy(user, nvram_safe_get("ddns_username_x"));
	strcpy(passwd, nvram_safe_get("ddns_passwd_x"));
	strcpy(host, nvram_safe_get("ddns_hostname_x"));
	strcpy(service, "");
			
	if (strcmp(server, "WWW.DYNDNS.ORG")==0)
		strcpy(service, "dyndns");			
	else if (strcmp(server, "WWW.DYNDNS.ORG(CUSTOM)")==0)
		strcpy(service, "dyndns-custom");			
	else if (strcmp(server, "WWW.DYNDNS.ORG(STATIC)")==0)
		strcpy(service, "dyndns-static");			
	else if (strcmp(server, "WWW.TZO.COM")==0)
		strcpy(service, "tzo");			
	else if (strcmp(server, "WWW.ZONEEDIT.COM")==0)
		strcpy(service, "zoneedit");
	else if (strcmp(server, "WWW.JUSTLINUX.COM")==0)
		strcpy(service, "justlinux");
	else if (strcmp(server, "WWW.EASYDNS.COM")==0)
		strcpy(service, "easydns");
	else if (strcmp(server, "WWW.DNSOMATIC.COM")==0)
		strcpy(service, "dnsomatic");
	else strcpy(service, "dyndns");
			
	sprintf(usrstr, "%s:%s", user, passwd);
	
	if (nvram_match("wan_proto", "pppoe") ||
	    nvram_match("wan_proto", "pptp")  ||
	    nvram_match("wan_proto", "l2tp"))
	{
		strcpy(wan_ifname, nvram_safe_get("wan0_pppoe_ifname"));
	}
	else
	{
		strcpy(wan_ifname, nvram_safe_get("wan0_ifname"));
	}	

	dprintf("wan_ifname: %s\n\n\n\n", wan_ifname);

#ifdef DDNSCONF
	if (!(fp = fopen("/etc/ddns.conf", "w"))) {
		perror("/etc/ddns.conf");
		return errno;
	}
	fprintf(fp, "service-type=%s\n", service);
	fprintf(fp, "interface=%s\n", wan_ifname);
	fprintf(fp, "user=%s\n", usrstr);
	fprintf(fp, "host=%s\n", host);
	if (wild) fprintf(fp, "wildcard\n");
	fappend("/usr/local/etc/ddns.conf", fp);
	fclose(fp);
#endif
	if (strlen(service)>0)
	{
		char *ddns_argv[] = {"ez-ipupdate", 
#ifdef DDNSDAEMON
		"-d", "-1",
#endif
#ifdef DDNSCONF
		"-c", "/etc/ddns.conf",
#else
		"-S", service,
	        "-i", wan_ifname,
 		"-u", usrstr,
		"-h", host,
		wild ? "-w" : NULL,
#endif
		"-e", "/sbin/ddns_updated",
		"-b", "/tmp/ddns.cache",
		NULL};	
		pid_t pid;

		dprintf("ddns update %s %s\n", server, service);
		nvram_unset("ddns_cache");
		nvram_unset("ddns_ipaddr");
		nvram_unset("ddns_status");
#ifdef DDNSDAEMON
		eval("killall", "-SIGQUIT", "ez-ipupdate");
#else
		eval("killall", "ez-ipupdate");
#endif
		_eval(ddns_argv, NULL, 0, &pid);
	}
	return 0;
}

int 
stop_ddns(void)
{
#ifdef DDNSDAEMON
	int ret = eval("killall", "-SIGQUIT", "ez-ipupdate");
#else
	int ret = eval("killall", "ez-ipupdate");
#endif
	dprintf("done\n");
	return ret;
}

int 
start_logger(void)
{		
	pid_t pid;

#if 0
	if (nvram_match("router_disable", "1"))
		return 0;
#endif

	char *syslogd_argv[] = {"syslogd", "-m", "0", "-O", "/tmp/syslog.log", "-S",
		"-l", "7", "-b", "2", "-R", nvram_safe_get("log_ipaddr"), "-L", NULL};
	char *klogd_argv[] = {"klogd", NULL};

	syslogd_argv[7] = nvram_get("log_level_x") ? : "7";

	if (!*syslogd_argv[11])
		syslogd_argv[10] = NULL;
		
	_eval(syslogd_argv, NULL, 0, &pid);
	_eval(klogd_argv, NULL, 0, &pid);
	// remote log not easy to ok
	sleep(1);
	return 0;
}

int
stop_logger(void)
{
	int ret1 = eval("killall", "klogd");
	int ret2 = eval("killall", "syslogd");

	dprintf("done\n");
	return (ret1|ret2);
}

int 
start_misc(void)
{ 
	char *infosvr_argv[] = {"infosvr", "br0", NULL};
	char *watchdog_argv[] = {"watchdog", NULL};
	pid_t pid;
	
	if (!nvram_invmatch("infosvr_enable", "1"))
		_eval(infosvr_argv, NULL, 0, &pid);
	_eval(watchdog_argv, NULL, 0, &pid);

	/* try to adjust wifi tx power */
	if (nvram_invmatch("wl_radio_power_x", ""))
	{
		eval("wl", "txpwr1", "-m", nvram_safe_get("wl_radio_power_x"), "-o");
	}

#if 1
	// for all product, fix antdiv
	if (eval("wl", "antdiv", "0") == 0)
		eval("wl", "txant", "0");
#endif

	return 0;
}

int
stop_misc(void)
{
	int ret;

	eval("killall", "infosvr");
	ret = eval("killall", "watchdog");
	stop_ntpc();

	dprintf("done\n");
	return(ret);
}

#ifndef USB_SUPPORT
int start_usb(void)
{
	return 0;
}

int stop_usb(void)
{
	return 0;
}

int hotplug_usb(void)
{
	return 0;
}
#else

int start_nfsd(void)
{
	struct stat	st_buf;
	FILE 		*fp;

	/* create directories/files */
	mkdir("/var/lib", 0755);
	mkdir("/var/lib/nfs", 0755);
	close(creat("/var/lib/nfs/etab", 0644));
	close(creat("/var/lib/nfs/xtab", 0644));
	close(creat("/var/lib/nfs/rmtab", 0644));
	
	/* create /etc/exports, if it does not exists yet */
	if (stat("/etc/exports", &st_buf) != 0) 
	{
		int i, count;
		char tmp[] = "usb_nfslist_xXXXXX";
		
		if ((fp = fopen("/etc/exports", "w")) == NULL) {
			perror("/etc/exports");
			return 1;
		}
		
		fprintf(fp, "# automagically generated from web settings\n");

		for (i = 0, count = atoi(nvram_safe_get("usb_nfsnum_x")); i < count; i++) 
		{
			sprintf(tmp, "usb_nfslist_x%d", i);
			if (nvram_safe_get(tmp)[0] == '/')
				fprintf(fp, "%s\n", nvram_safe_get(tmp));
			else	fprintf(fp, "/tmp/harddisk/%s\n", nvram_safe_get(tmp));
		}
		fappend("/usr/local/etc/exports", fp);
		fclose(fp);
	}

	eval("/usr/sbin/portmap");
	eval("/usr/sbin/lockd");
	eval("/usr/sbin/statd");
	eval("/usr/sbin/nfsd");
	eval("/usr/sbin/mountd");
	eval("/usr/sbin/exportfs", "-a");

	return 0;	
}

int restart_nfsd(void)
{
	eval("/usr/sbin/exportfs", "-au");
	eval("/usr/sbin/exportfs", "-a");

	return 0;	
}

int 
start_usb(void)
{
	eval("insmod", "usbcore");
	eval("insmod", "usb-ohci");
	eval("insmod", "usb-uhci");
	if (nvram_invmatch("usb20_disable_x", "1"))
		eval("insmod", "ehci-hcd");

	/* mount usbfs */
	mount("usbfs", "/proc/bus/usb", "usbfs", MS_MGC_VAL, NULL);

#ifdef PRINTER_SUPPORT
#ifdef PARPORT_SUPPORT	
	symlink("/dev/printers/0", "/dev/lp0");
	symlink("/dev/lp0", "/tmp/lp0");
	eval("insmod", "parport.o");
	eval("insmod", "parport_splink.o");
	eval("insmod", "lp.o");
#endif		
	eval("insmod", "printer.o");
	mkdir("/var/state", 0777);
	mkdir("/var/state/parport", 0777);
	if (!nvram_invmatch("lpr_enable", "1"))
	{
		char *lpd_argv[]={"lpd", NULL};
		pid_t pid;

		sleep(1);
		_eval(lpd_argv, ">/dev/null", 0, &pid);	
	}
	if (!nvram_invmatch("raw_enable", "1"))
	{
		eval("p910nd", "-f", "/dev/usb/lp0", "0");
#ifdef PARPORT_SUPPORT	
		eval("p910nd", "-f", "/dev/printers/0", "1");
#endif
	}
#endif	
	if (!nvram_invmatch("audio_enable", "1"))
	{
#ifdef AUDIO_SUPPORT
		eval("insmod", "soundcore");
		eval("insmod", "audio");
		start_audio();
	}
#endif
#ifdef WEBCAM_SUPPORT	
	if (nvram_invmatch("usb_webenable_x", "0"))
	{	
		eval("insmod", "videodev.o");

		// link video 
		symlink("/dev/v4l/video0", "/dev/video");
		start_rcamd();
	}
#endif
#ifdef MASSSTORAGE_SUPPORT
	mkdir("/tmp/mnt", 0755);

	if (!nvram_match("usb_storage_x", "0"))
	{
		eval("insmod", "scsi_mod.o");
		eval("insmod", "sd_mod.o");
		eval("insmod", "usb-storage.o");
	}	
#endif
	if (nvram_match("usb_nfsenable_x", "1"))
	{	
		eval("insmod", "sunrpc");
		eval("insmod", "lockd");
		eval("insmod", "nfsd");
		
		start_nfsd();
	}
	return 0;
}

int
stop_usb(void)
{
	if (nvram_match("usb_nfsenable_x", "1"))
	{	
		eval("killall", "mountd");
		eval("killall", "-9", "nfsd");
		eval("killall", "-9", "lockd");
		eval("killall", "portmap");
		
		eval("rmmod", "nfsd");
		eval("rmmod", "lockd");
		eval("rmmod", "sunrpc");
	}
	
#ifdef MASSSTORAGE_SUPPORT
	if (!nvram_match("usb_storage_x", "0"))
	{
		eval("killall", "stupid-fptd");
		eval("killall", "smbd");
		eval("killall", "nmbd");
		umount_all_part(NULL, -1);
		eval("rmmod", "usb-storage");
		eval("rmmod", "sd_mod");
		eval("rmmod", "scsi_mod");
	}
#endif
#ifdef WEBCAM_SUPPORT	
	if (nvram_invmatch("usb_webenable_x", "0"))
	{
		stop_rcamd();	
		eval("killall", "rcamd");
		eval("killall", "sendmail");
		eval("rmmod", "pwc");
		eval("rmmod", "ov511_decomp");
		eval("rmmod", "ov518_decomp");
		eval("rmmod", "ov51x");
		eval("rmmod", "i2c-core");
		eval("rmmod", "videodev");
	}
#endif
#ifdef AUDIO_SUPPORT
	eval("rmmod", "audio");
	eval("rmmod", "soundcore");
	stop_audio();
#endif
#ifdef PRINTER_SUPPORT	
	eval("killall", "lpd");
	eval("killall", "p910nd");
	eval("rmmod", "printer");
#ifdef PARPORT_SUPPORT
	eval("rmmod", "lp.o");
	eval("rmmod", "parport_splink.o");
	eval("rmmod", "parport.o");
#endif
#endif	

	umount("/proc/bus/usb");

	eval("rmmod", "ehci-hcd");
	eval("rmmod", "usb-uhci");
	eval("rmmod", "usb-ohci");
	eval("rmmod", "usbcore");
	return 0;
}

void start_script(void)
{
//	pid_t pid;
	FILE *fp;
	char *script;
	char runbuf[512];

	script = nvram_safe_get("usb_ftpscript_x");
	if (strlen(script)==0) return;
	sprintf(runbuf, "/tmp/harddisk/%s", script);
	fp=fopen(runbuf, "r");
	
	if (fp)
	{
		fclose(fp);

		//char *script_argv[]={"/tmp/harddisk/init.usb", NULL};
		//_eval(script_argv, NULL, 0, &pid);

		logmessage("USB storage", "start user specified script");
		eval(runbuf);
	}	
}

/* get full storage path */
char *nvram_storage_path(char *var)
{
	static char buf[256];
	char *val = nvram_safe_get(var);
	int len = sprintf(buf, "%s%s", 
		val[0] == '/' ? "" : "/tmp/harddisk/", val);
	
	if (len > 1 && buf[len - 1] == '/')
		buf[len - 1] = 0;

	return buf;
}

int restart_ftpd()
{
	char vsftpd_users[] = "/etc/vsftpd.users";
	char vsftpd_passwd[] = "/etc/vsftpd.passwd";
	int i, count;

	char tmp[256];
	FILE *fp, *f;

	eval("killall", "vsftpd");
	
	mkdir_if_none(vsftpd_users);

	if ((fp = fopen("/etc/vsftpd.conf", "w")) == NULL)
		return 1;

	if (nvram_match("usb_ftpsuper_x", "1"))
	{
		/* rights */
		sprintf(tmp, "%s/%s", vsftpd_users,
			nvram_get("http_username") ? : "admin");
		if ((f = fopen(tmp, "w")))
		{
			fprintf(f, 
				"dirlist_enable=yes\n"
				"write_enable=yes\n"
				"download_enable=yes\n");
			fclose(f);
		}
	}

	if (nvram_invmatch("usb_ftpanonymous_x", "0"))
	{
		fprintf(fp, 
			"anon_allow_writable_root=yes\n"
			"anon_world_readable_only=no\n"
			"anon_umask=022\n");
		
		/* rights */
		sprintf(tmp, "%s/ftp", vsftpd_users);
		if ((f = fopen(tmp, "w")))
		{
			if (nvram_match("usb_ftpdirlist_x", "0"))
				fprintf(f, "dirlist_enable=yes\n");
			if (nvram_match("usb_ftpanonymous_x", "1") || 
			    nvram_match("usb_ftpanonymous_x", "3"))
				fprintf(f, "write_enable=yes\n");
			if (nvram_match("usb_ftpanonymous_x", "1") || 
			    nvram_match("usb_ftpanonymous_x", "2"))
				fprintf(f, "download_enable=yes\n");
			fclose(f);
		}
		if (nvram_match("usb_ftpanonymous_x", "1") || 
		    nvram_match("usb_ftpanonymous_x", "3"))
			fprintf(fp, 
				"anon_upload_enable=yes\n"
				"anon_mkdir_write_enable=yes\n"
				"anon_other_write_enable=yes\n");
	} else {
		fprintf(fp, "anonymous_enable=no\n");
	}
	
	fprintf(fp,
		"dirmessage_enable=yes\n"
		"download_enable=no\n"
		"dirlist_enable=no\n"
		"hide_ids=yes\n"
		"syslog_enable=yes\n"
		"local_enable=yes\n"
		"local_umask=022\n"
		"chmod_enable=no\n"
		"chroot_local_user=yes\n"
		"check_shell=no\n"
		"user_config_dir=%s\n"
		"passwd_file=%s\n",
		vsftpd_users, vsftpd_passwd);

	fprintf(fp,
#ifdef __CONFIG_IPV6__
		"listen_ipv6=yes\n"
#else
		"listen=yes\n"
#endif
		"listen_port=%s\nbackground=yes\n",
		nvram_get("usb_ftpport_x") ? : "21");
	fprintf(fp, "max_clients=%s\n", nvram_get("usb_ftpmax_x") ? : "0");

	/* fprintf(fp, "login-timeout=%s\n", nvram_safe_get("usb_ftptimeout_x")); */
	fprintf(fp, "idle_session_timeout=%s\n", nvram_get("usb_ftpstaytimeout_x") ? : "300");

	if (nvram_match("usb_smbcset_x", "utf8"))
		fprintf(fp, "utf8=yes\n");

	/* ntfs does not support sendfile at the moment */
	fprintf(fp, "use_sendfile=no\n");

	/* bandwidth */
	fprintf(fp, "anon_max_rate=%d\nlocal_max_rate=%d\n",
		atoi(nvram_safe_get("usb_ftpanonrate_x")) * 1024,
		atoi(nvram_safe_get("usb_ftprate_x")) * 1024);
	
	fclose(fp);
	
	/* prepare passwd file and default users */
	if ((fp = fopen(vsftpd_passwd, "w")) == NULL)
		return 2;

	fprintf(fp, /* anonymous, admin, nobody */
		"ftp:x:0:0:ftp:%s:/sbin/nologin\n"
		"%s:%s:0:0:root:/tmp/mnt/:/sbin/nologin\n"
		"nobody:x:99:99:nobody:/usr/share/empty:/sbin/nologin\n",
		nvram_storage_path("usb_ftpanonroot_x"), 
		nvram_get("http_username") ? : "admin",
		nvram_match("usb_ftpsuper_x", "1") ? 
			crypt(nvram_get("http_passwd") ? : "admin", "$1$") : "x");

	for (i = 0, count = atoi(nvram_safe_get("usb_ftpnum_x")); i < count; i++) 
	{
		char *user = (sprintf(tmp, "usb_ftpusername_x%d", i), nvram_get(tmp));
		char *pass = (sprintf(tmp, "usb_ftppasswd_x%d", i), nvram_get(tmp));
		char *rights = (sprintf(tmp, "usb_ftprights_x%d", i), nvram_get(tmp));
		
		if (user && pass && rights)
		{
			/* directory */
			if (strncmp(rights, "Private", 7) == 0)
			{
				sprintf(tmp, "%s/%s", nvram_storage_path("usb_ftppvtroot_x"), user);
				mkdir_if_none(tmp);
			}
			else	sprintf(tmp, "%s", nvram_storage_path("usb_ftppubroot_x"));


			fprintf(fp, "%s:%s:0:0:%s:%s:/sbin/nologin\n",
				user, crypt(pass, "$1$"), user, tmp);

			/* rights */
			sprintf(tmp, "%s/%s", vsftpd_users, user);
			if ((f = fopen(tmp, "w")))
			{
				if (nvram_invmatch("usb_ftpdirlist_x", "1"))
					strcpy(tmp, "dirlist_enable=yes\n");
				if (strstr(rights, "Read") || !strcmp(rights, "Private"))
					strcat(tmp, "download_enable=yes\n");
				if (strstr(rights, "Write") || !strncmp(rights, "Private", 7))
					strcat(tmp, "write_enable=yes\n");
					
				fputs(tmp, f);
				fclose(f);
			}
		}
	}
	
	fclose(fp);
	
	eval("/usr/sbin/vsftpd");
	return 0;
}

void restart_smbd()
{
	FILE *fp;
	DIR *dir = NULL;
	struct dirent *dp;
	
	kill_pidfile("/var/lock/nmbd.pid");
	if (!kill_pidfile("/var/lock/smbd.pid"))
		sleep(2);	/* wait for smbd to finish */

	if ((fp = fopen("/etc/smb.conf", "w")) == NULL) {
		perror("/etc/smb.conf");
		return;
	}
		
	fprintf(fp, "[global]\n"
		"\tinterfaces = %s\n"
		"\tbind interfaces only = yes\n"
		"\tworkgroup = %s\n"
		"\tserver string = %s\n"
		"\tguest account = nobody\n"
		"\tsecurity = share\n"
		"\tbrowseable = yes\n"
		"\tguest ok = yes\n"
		"\tguest only = yes\n"
		"\tlog level = 1\n"
		"\tmax log size = 100\n"
		"\tencrypt passwords = no\n"
		"\tpreserve case = yes\n"
		"\tshort preserve case = yes\n",
		nvram_get("lan_ifname") ? : "br0",
		nvram_get("usb_smbwrkgrp_x") ? : "WORKGROUP",
		nvram_get("productid") ? : "Samba");

	if (nvram_invmatch("usb_smbcpage_x", ""))
		fprintf(fp, "\tclient code page = %s\n", nvram_get("usb_smbcpage_x"));
	if (nvram_match("usb_smbcset_x", "utf8"))
		fprintf(fp, "\tcoding system = utf8\n");
	else if (nvram_invmatch("usb_smbcset_x", ""))
		fprintf(fp, "\tcharacter set = %s\n", nvram_get("usb_smbcset_x"));

	if (nvram_match("usb_smbenable_x", "1"))
	{
		fprintf(fp, "\n[share]\n"
			"\tpath = /tmp/harddisk\n");
		
		if (nvram_match("usb_smbhidden_x", "1"))
			fprintf(fp, "\n[share$]\n"
				"\tpath = /tmp/harddisk\n"
				"\tbrowseable = no\n");
		
		if (nvram_match("usb_smbhidden_x", "1") || 
		    nvram_match("usb_smbhidden_x", "2"))
			fprintf(fp, 
				"\twritable = yes\n"
				"\tforce user = %s\n",
				nvram_get("http_username") ? : "root");
	}

	/* share everything below /tmp/mnt */
	if (nvram_match("usb_smbenable_x", "2") && (dir = opendir("/tmp/mnt")))
		while ((dp = readdir(dir)))
	{
		if (strcmp(dp->d_name, ".") && strcmp(dp->d_name, ".."))
		{
			fprintf(fp, "\n[%s]\n"
				"\tpath = /tmp/mnt/%s\n", dp->d_name, dp->d_name);

			if (nvram_match("usb_smbhidden_x", "1"))
				fprintf(fp, "\n[%s$]\n"
					"\tpath = /tmp/mnt/%s\n"
					"\tbrowseable = no\n", dp->d_name, dp->d_name);
		
			if (nvram_match("usb_smbhidden_x", "1") || nvram_match("usb_smbhidden_x", "2"))
				fprintf(fp, 
					"\twritable = yes\n"
					"\tforce user = %s\n",
					nvram_get("http_username") ? : "root");
		}
	}

	if (dir) closedir(dir);

	if (nvram_match("usb_smbenable_x", "3"))
	{
		int i, count;
		char tmp[] = "usb_smblist_xXXXXX";
		
		for (i = 0, count = atoi(nvram_safe_get("usb_smbnum_x")); i < count; i++) 
		{
			/* share name */
			sprintf(tmp, "usb_smbshare_x%d", i);
			if (!nvram_invmatch(tmp, ""))
				continue;
			fprintf(fp, "\n[%s]\n", nvram_get(tmp));
		
			/* path */
			sprintf(tmp, "usb_smbdir_x%d", i);
			fprintf(fp, "\tpath = %s\n", 
				nvram_storage_path(tmp));

			/* access level */
			sprintf(tmp, "usb_smblevel_x%d", i);
			if (nvram_match(tmp, "Read/Write"))
				fprintf(fp, 
					"\twritable = yes\n"
					"\tforce user = %s\n",
					nvram_get("http_username") ? : "root");
			/* comment */
			sprintf(tmp, "usb_smbdesc_x%d", i);
			if (nvram_invmatch(tmp, ""))
				fprintf(fp, "\tcomment = %s\n", nvram_get(tmp));
		}
	}
	fappend("/usr/local/etc/smb.conf", fp);
	fclose(fp);

	eval("/usr/sbin/nmbd", "-D");
	eval("/usr/sbin/smbd", "-D");
}

int
hotplug_usb_webcam(char *product, int webflag)
{
	char *rcamd_argv[]={"rcamd", 
				"-p", nvram_safe_get("usb_webactivex_x"),
				"-s", nvram_safe_get("usb_webfresh_x"),
				"-z", nvram_safe_get("time_zone"),
				"-a", nvram_safe_get("usb_websecurity_x"),
				NULL, NULL,	// Model -t
				NULL, NULL,	// Record Time -r
				NULL, NULL, 	// Image Size -f
				NULL, NULL, 	// Sense Vlaue -m 
				NULL, NULL, 	// Sense Limit -c
				NULL, NULL,
				NULL};
	char **arg;
	pid_t pid;

	if (nvram_match("usb_webenable_x", "0") || strlen(product)==0 || webflag==0) return 1;
	for (arg = rcamd_argv; *arg; arg++);
	
	if (webflag == WEB_PWCWEB)
	{
		eval("insmod", "pwc", "power_save=0", "size=vga");
		eval("insmod", "pwcx");
		nvram_set("usb_webdriver_x", "0");

		*arg++ = "-t";
		*arg++ = "0";
	}
	else
	{
		eval("insmod", "i2c-core.o");
		eval("insmod", "ov51x.o");
		eval("insmod", "ov511_decomp.o");
		eval("insmod", "ov518_decomp.o");
		nvram_set("usb_webdriver_x", "1");

		if (strstr(product, "8519"))
		{
			*arg++ = "-t";
			*arg++ = "2";
		}
		else
		{			
			*arg++ = "-t";
			*arg++ = "1";
		}
	}

	// start web cam
	if (nvram_match("usb_webmode_x", "0")) // ActiveX only
	{
		*arg++ = "-r";
		*arg++ = "60";
	}
	else 
	{
		*arg++ = "-r";
		*arg++ = "0";
	}
		
	// image size
	if (nvram_match("usb_webimage_x", "0"))
	{
		*arg++ = "-f";

		*arg++="640x480";
	}
	else if (nvram_match("usb_webimage_x", "1"))
	{
		*arg++ = "-f";
		*arg++="320x240";
	}
	else if (nvram_match("usb_webimage_x", "2"))
	{
		*arg++ = "-f";
		if (webflag==WEB_PWCWEB) *arg++="160x120";
		else *arg++="176x144";
	}
	else
	{
		*arg++ = "-f";
		*arg++ = "80x60";
	}


	if (nvram_match("usb_websense_x", "0"))
	{	
		*arg++ = "-m";
		*arg++ = "150";
		*arg++ = "-c";
		*arg++ = "100";
	}
	else if (nvram_match("usb_websense_x", "1"))
	{
		*arg++ = "-m";
		*arg++ = "100";
		*arg++ = "-c";
		*arg++ = "100";
	}
	else
	{	
		*arg++ = "-m";
		*arg++ = "50";
		*arg++ = "-c";
		*arg++ = "100";
	}
	

	//*arg++="-d";
	//*arg++="7";
	mkdir("/tmp/webcam", 0777);	
	chdir("/tmp/webcam");

	symlink("/www/Advanced_ShowTime_Widzard.asp", "/tmp/webcam/index.asp");
	symlink("/www/Advanced_ShowTime_Widzard.asp", "/tmp/webcam/ShowWebCam.asp");
	symlink("/www/ShowWebCamPic.asp", "/tmp/webcam/ShowWebCamPic.asp");
	symlink("/www/graph", "/tmp/webcam/graph");
	symlink("/www/general.js", "/tmp/webcam/general.js");
	symlink("/www/overlib.js", "/tmp/webcam/overlib.js");
	symlink("/www/style.css", "/tmp/webcam/style.css");
	symlink("/www/netcam_mfc_activeX.cab", "/tmp/webcam/netcam_mfc_activeX.cab");
	symlink("/var/tmp/display.jpg", "/tmp/webcam/display.jpg");

	//char *httpd_argv[]={"httpd", 
	//			nvram_safe_get("wan0_ifname"), 
	//			nvram_safe_get("usb_webhttpport_x"),
	//			NULL};
	//_eval(httpd_argv, NULL, 0, &pid);
	eval("httpd", nvram_safe_get("wan0_ifname"), nvram_safe_get("usb_webhttpport_x"));
	chdir("/");
	_eval(rcamd_argv, ">/dev/null", 0, &pid);

	return 0;
}

int
remove_webcam_main(int webflag)
{
	if (webflag == WEB_PWCWEB)
	{
		eval("rmmod", "pwc");
	}
	else
	{
		eval("rmmod", "i2c-core");
		eval("rmmod", "ov511_decomp");
		eval("rmmod", "ov518_decomp");
		eval("rmmod", "ov51x");
	}
	nvram_set("usb_webdriver_x", "");
	return 0;
}


int
remove_usb_webcam(char *product, int webflag)
{
	char pidfile[32];
	sprintf(pidfile, "/var/run/httpd-%s.pid", nvram_safe_get("usb_webhttpport_x"));

	kill_pidfile(pidfile);
	kill_pidfile("/var/run/rcamd.pid");

	if (webflag == WEB_PWCWEB)
	{
		eval("rmmod", "pwc");
	}
	else
	{
		eval("rmmod", "i2c-core");
		eval("rmmod", "ov511_decomp");
		eval("rmmod", "ov518_decomp");
		eval("rmmod", "ov51x");
	}
	nvram_set("usb_webdriver_x", "");

	return 0;
}


int
start_rcamd(void)
{
	char *rcamd_argv[] = {"rcamdmain", NULL};
	pid_t pid;

	return _eval(rcamd_argv, NULL, 0, &pid);
}

int
stop_rcamd(void)
{
	eval("killall", "rcamdmain");
	return 0;
}


/*to unmount all partitions*/
static int
umount_all_part(char *product, int scsi_host_no)
{
	DIR *dir_to_open, *usb_dev_disc;
	struct dirent *dp;
	char umount_dir[128];
	int usb_disc_0 = 0;

	if (scsi_host_no != -1)
	{
	    // umount all partitions from specific scsi host only
	    char discs_path[64], parts[128];
	    char scsi_dev_link[256];
	    char buf[256];
	    FILE *part_fp;
	    struct mntent *mnt;
	    int t_host_no;
	    int len;

	    if ((usb_dev_disc = opendir("/dev/discs")))
    	    {
		while ((dp = readdir(usb_dev_disc))) {
		    if (strncmp(dp->d_name, "disc", 4) != 0)
                    	continue;
		    snprintf(discs_path, sizeof(discs_path), "%s/%s", "/dev/discs", dp->d_name);
		    len = readlink(discs_path, scsi_dev_link, sizeof(scsi_dev_link) - 1);
                    if (len < 0)
                    	continue;

		    scsi_dev_link[len] = '\0';
		    if (strncmp(scsi_dev_link, "../scsi/host", 12))
                    	continue;
		    t_host_no = atoi(scsi_dev_link + 12);
		    if (t_host_no != scsi_host_no)
                    	continue;

		    /* We have found a disc that is on this controller.
            	     * Loop thru all the partitions on this disc.
	             */
		    if ((part_fp = fopen("/proc/partitions", "r")))
		    {
			while (fgets(buf, sizeof(buf) - 1, part_fp))
			{
			    if (sscanf(buf, " %*s %*s %*s %s", parts) == 1)
			    {
				if (strncmp(parts, scsi_dev_link+3, len-3) == 0
				     && strncmp(parts + (len-3), "/part", 5) == 0)
				{
				    snprintf(umount_dir, sizeof(umount_dir), "%s%s", discs_path, parts + (len-3));
				    if ((mnt = findmntent(umount_dir)))
				    {
					if (!umount(mnt->mnt_dir))
					    unlink(mnt->mnt_dir);
				    }
				}
			    }
			}
			fclose(part_fp);
		    } /* partitions loop */

		}
		closedir(usb_dev_disc);
            }
	}
	else if ((dir_to_open = opendir("/tmp/mnt")))
	{
		while ((dp=readdir(dir_to_open)))
		{
		    if (strncmp(dp->d_name, "disc", 4) == 0)
		    {
			sprintf(umount_dir, "/tmp/mnt/%s", dp->d_name);
			if (!umount(umount_dir))
				unlink(umount_dir);
		    }
		}
		closedir(dir_to_open);
		usb_disc_0 = 1;
	}

	if (usb_disc_0)
		unlink("/tmp/harddisk");

	return 0;
}

/* remove usb mass storage */
int
remove_usb_mass(char *product, int scsi_host_no)
{
	if (product==NULL || nvram_match("usb_ftp_device", product))
	{
		if (nvram_invmatch("usb_ftpenable_x", "0")) {
			eval("killall", "stupid-ftpd");
			eval("killall", "vsftpd");
		}
		if (nvram_invmatch("usb_smbenable_x", "0")) {
			eval("killall", "smbd");
			eval("killall", "nmbd");
		}

		sleep(1);
		umount_all_part(product, scsi_host_no);
		nvram_set("usb_ftp_device", "");
		logmessage("USB storage", "removed");
	}
	return 0;
}

int mkdir_if_none(char *dir)
{
	DIR *dp;
	if(!(dp=opendir(dir)))
		return(mkdir(dir, 0777));
	closedir(dp);
	return 0;
}

int
remove_storage_main(int scsi_host_no)
{
	return remove_usb_mass(NULL, scsi_host_no);
}

/* stollen from the e2fsprogs/ismounted.c */
static struct mntent *findmntent(char *file)
{
	struct mntent 	*mnt;
	struct stat	st_buf;
	dev_t		file_dev=0, file_rdev=0;
	ino_t		file_ino=0;
	FILE 		*f;
	
	if ((f = setmntent ("/proc/mounts", "r")) == NULL)
		return NULL;

	if (stat(file, &st_buf) == 0) {
		if (S_ISBLK(st_buf.st_mode)) {
			file_rdev = st_buf.st_rdev;
		} else {
			file_dev = st_buf.st_dev;
			file_ino = st_buf.st_ino;
		}
	}
	while ((mnt = getmntent (f)) != NULL) {
		if (strcmp(file, mnt->mnt_fsname) == 0)
			break;
		if (stat(mnt->mnt_fsname, &st_buf) == 0) {
			if (S_ISBLK(st_buf.st_mode)) {
				if (file_rdev && (file_rdev == st_buf.st_rdev))
					break;
			} else {
				if (file_dev && ((file_dev == st_buf.st_dev) &&
						 (file_ino == st_buf.st_ino)))
					break;
			}
		}
	}

	fclose(f);
	return mnt;
}

char *detect_fs_type(char *device)
{
	int fd;
	unsigned char buf[4096];
	
	if ((fd = open(device, O_RDONLY)) < 0)
		return NULL;
		
	if (read(fd, buf, sizeof(buf)) != sizeof(buf))
	{
		close(fd);
		return NULL;
	}
	
	close(fd);
	
	/* first check for mbr */
	if (*device && device[strlen(device) - 1] > '9' &&
	    buf[510] == 0x55 && buf[511] == 0xAA && /* signature */
	    ((buf[0x1be] | buf[0x1ce] | buf[0x1de] | buf[0x1ee]) & 0x7f) == 0) /* boot flags */
	{
		return "mbr";
	} else
	/* detect swap */
	if (memcmp(buf + 4086, "SWAPSPACE2", 10) == 0 ||
	    memcmp(buf + 4086, "SWAP-SPACE", 10) == 0)
	{
		return "swap";
	} else
	/* detect ext2/3 */
	if (buf[0x438] == 0x53 && buf[0x439] == 0xEF)
	{
		return	((buf[0x460] & 0x0008 /* JOURNAL_DEV */) != 0 ||
			 (buf[0x45c] & 0x0004 /* HAS_JOURNAL */) != 0) ? "ext3" : "ext2";
	} else 
	/* detect ntfs */
	if (buf[510] == 0x55 && buf[511] == 0xAA && /* signature */
	    memcmp(buf + 3, "NTFS    ", 8) == 0)
	{
		return "ntfs";
	} else
	/* detect vfat */
	if (buf[510] == 0x55 && buf[511] == 0xAA && /* signature */
	    buf[11] == 0 && buf[12] >= 1 && buf[12] <= 8 /* sector size 512 - 4096 */ &&
	    buf[13] != 0 && (buf[13] & (buf[13] - 1)) == 0) /* sectors per cluster */
	{
	 	return "vfat";
	}

	return NULL;
}

#define MOUNT_VAL_FAIL 	0
#define MOUNT_VAL_RONLY	1
#define MOUNT_VAL_RW 	2

int
mount_r(char *mnt_dev, char *mnt_dir)
{
	struct mntent *mnt = findmntent(mnt_dev);
	char *type;
	
	if (mnt) {
		return strcmp(mnt->mnt_dir, mnt_dir) ? 
			MOUNT_VAL_FAIL : MOUNT_VAL_RW;
	}
	
	if ((type = detect_fs_type(mnt_dev))) 
	{
		char options[64];
		unsigned long flags = MS_NOATIME;

		options[0] = '\0';
		
		if (strcmp(type, "swap") == 0 || strcmp(type, "mbr") == 0)
			flags = 0; /* not a mountable partition */
		else if (strcmp(type, "vfat") == 0)
		{
			int l = 0;

			if (nvram_invmatch("usb_vfat_options", ""))
				l = snprintf(options, 64, "%s", nvram_get("usb_vfat_options"));
			if (nvram_invmatch("usb_smbcset_x", ""))
				l += snprintf(options + l, 64-l, ",iocharset=%s%s" + ((l>0) ? 0 : 1),
					isdigit(nvram_get("usb_smbcset_x")[0]) ? "cp" : "",
						nvram_get("usb_smbcset_x"));
			if (nvram_invmatch("usb_smbcpage_x", ""))
				l += snprintf(options + l, 64-l, ",codepage=%s" + ((l>0) ? 0 : 1), 
					nvram_get("usb_smbcpage_x"));
			options[l+1] = '\0';
		}
		else if (strcmp(type, "ntfs") == 0)
		{
			flags = MS_RDONLY;
			if (nvram_invmatch("usb_smbcset_x", ""))
				sprintf(options, "iocharset=%s%s", 
					isdigit(nvram_get("usb_smbcset_x")[0]) ? "cp" : "",
						nvram_get("usb_smbcset_x"));
		}

		if (flags && !mkdir_if_none(mnt_dir) &&
			!mount(mnt_dev, mnt_dir, type, flags, options[0] ? options : NULL)) 
		{
			logmessage("USB storage", "%s%s fs at %s mounted to %s", 
				type, (flags & MS_RDONLY) ? "(ro)" : "", mnt_dev, mnt_dir);
			return (flags & MS_RDONLY) ? MOUNT_VAL_RONLY : MOUNT_VAL_RW;
		}

		rmdir(mnt_dir);
	}
	
	return MOUNT_VAL_FAIL;
}

/* insert usb mass storage */
int hotplug_usb_mass(char *product)
{	
	DIR *dir_to_open, *usb_dev_disc, *usb_dev_part;
	char usb_disc[128], mnt_dev[128], mnt_dir[128];
	struct dirent *dp, *dp_disc;

	dir_to_open = usb_dev_disc = usb_dev_part = NULL;
	
	// Mount USB to system
	if((usb_dev_disc = opendir("/dev/discs")))
	{
		eval("/usr/local/sbin/pre-mount", product);

		struct stat st_buf;
		if (stat("/etc/fstab", &st_buf) == 0) {
			eval("swapon", "-a");
			eval("mount", "-a");
		}
		
		while ((dp=readdir(usb_dev_disc)))
		{
			if(!strcmp(dp->d_name, "..") || !strcmp(dp->d_name, "."))
				continue;

			sprintf(usb_disc, "/dev/discs/%s", dp->d_name);

			if((usb_dev_part = opendir(usb_disc)))
			{
				while ((dp_disc=readdir(usb_dev_part)))
				{
					/* assume disc is the first entry */
					int disc = !strcmp(dp_disc->d_name, "disc");
					
					if (!strcmp(dp_disc->d_name, "..") || !strcmp(dp_disc->d_name, "."))
						continue;
					
					sprintf(mnt_dev, "/dev/discs/%s/%s", dp->d_name, dp_disc->d_name);

					if (disc) sprintf(mnt_dir, "/tmp/mnt/%s", dp->d_name);
					else sprintf(mnt_dir, "/tmp/mnt/%s_%s", dp->d_name, 
						dp_disc->d_name + (strncmp(dp_disc->d_name, "part", 4) ? 0 : 4));

					if (mount_r(mnt_dev, mnt_dir))
					{
						if (disc) break; /* no mbr -- no partitions */
					}
				}
			}
		}
		
		/* create /tmp/harddisk pointing to first partition */
		usb_disc[0] = 0;	/* alphabetical sort minimum */
		for (dir_to_open = opendir("/tmp/mnt");
			dir_to_open && (dp = readdir(dir_to_open)); )
		{
			if (!strncmp(dp->d_name, "disc", 4) &&
			    (usb_disc[0] == 0 || strcmp(dp->d_name, usb_disc) < 0))
				strcpy(usb_disc, dp->d_name);
		}
		
		if (usb_disc[0])
		{
			sprintf(mnt_dir, "/tmp/mnt/%s", usb_disc);
			symlink(mnt_dir, "/tmp/harddisk");

			nvram_set("usb_ftp_device", product);
			if (nvram_invmatch("usb_ftpenable_x", "0")) {
				restart_ftpd();
			}

			if (nvram_invmatch("usb_smbenable_x", "0") && nvram_invmatch("lan_hostname", "")) {
				restart_smbd();
			}

			if (nvram_match("usb_nfsenable_x", "1"))
			{	
				restart_nfsd();
			}
			
			//logmessage("USB storage", "attached");
			start_script();
		}

		//run script if any
		eval("/usr/local/sbin/post-mount", product, mnt_dir);
	}

	if(usb_dev_disc)
		closedir(usb_dev_disc);
	if(usb_dev_part)
		closedir(usb_dev_part);
	if(dir_to_open)
		closedir(dir_to_open);

	return 0;
}

/* plugging or removing usb device */
/* usbcore, usb-ohci, usb-ehci, printer are always there */
/* usb-storage, sd_mod, scsi_mod, videodev are there if functions are enabled */
/* pwc, ov511 i2c, depends on current status */

int
hotplug_usb(void)
{
	char *action, *interface, *product;
	int i;
	int isweb;
	char flag[6];

	if( !(interface = getenv("INTERFACE")) || !(action = getenv("ACTION")))
		return EINVAL;

	if ((product=getenv("PRODUCT")))
	{
		/* usb storage */
		if (strncmp(interface, "8/", 2) == 0)
		{
			char *scsi_host = getenv("SCSI_HOST");
			int scsi_host_no = -1;

			if (scsi_host)
				scsi_host_no = atoi(scsi_host);
			if (strcmp(action, "add") == 0)
				nvram_set("usb_storage_device", product);
			else
				remove_usb_mass(product, scsi_host_no);
			return 0;
		}

		if (strncmp(interface, "1/1", 3)==0)
		{
			// if the audio device is the same with web cam,
			// just skip it
			if (nvram_match("usb_web_device", product)) 
				return 0;
			isweb = WEB_AUDIO;
			goto usbhandler;
		}
		else if(strncmp(interface, "1/", 2)==0)
			return 0;

		/* ignore hubs */
		if (strncmp(interface, "9/", 2)==0)
			return 0;

		i=0;
		isweb = WEB_NONE;
		while(PWCLIST[i]!=NULL)
		{
			if (strstr(product, PWCLIST[i]))
			{
				isweb = WEB_PWCWEB;
				goto usbhandler;
			}
			i++;
		}
		i=0;
		while(OVLIST[i]!=NULL)
		{
			if (strstr(product, OVLIST[i]))
			{
				isweb = WEB_OVWEB;
				goto usbhandler; 
			}
			i++;
		}
	}
	else return 0;

usbhandler:	
	if (!strncasecmp(action, "add", 3))
	{		
		nvram_set("usb_device", "1");
		if (isweb==WEB_NONE)
		{
			/* old usb-storage handler */
		}
		else if (isweb==WEB_AUDIO)
		{
			if (nvram_match("usb_audio_device", ""))
				logmessage("USB audio", "attached");
			nvram_set("usb_audio_device", product);
			refresh_wave();
		}
		else
		{
			if (nvram_match("usb_web_device", ""))	
				logmessage("USB webcam", "attached");

			sprintf(flag, "%d", isweb);
			nvram_set("usb_web_device", product);
			nvram_set("usb_web_flag", flag);
			nvram_set("usb_webdriver_x", "");
		}
	}
	else 
	{
		if (isweb==WEB_NONE) // Treat it as USB Storage
		{
			/* old usb-storage handler */
		}
		else if(isweb==WEB_AUDIO)
		{
			remove_usb_audio(product);
			nvram_set("usb_audio_device", "");
		}
		else
		{
			if (nvram_invmatch("usb_web_device", ""))
			{	
				logmessage("USB webcam", "removed");
				nvram_set("usb_web_device", "");
				nvram_set("usb_web_flag", "");
			}
			remove_usb_webcam(product, isweb);
		}
	}
	return 0;
}
#endif	

/* stop necessary services for firmware upgrade */	
int
stop_service_main()
{
	stop_misc();

	preshutdown_system();
	stop_usb();

	/* nas is still needed for upgrade over WiFI with WPA enabled */
	/* stop_nas();*/

	stop_upnp();
	stop_snmpd();
	stop_logger();

	stop_dhcpd();
	stop_dns();

	dprintf("done\n");
	return 0;
}

int service_handle(void)
{
	char *service;
	char tmp[100], *str;
	int pid;
	char *ping_argv[] = { "ping", "-c2", "140.113.1.1", NULL};
	FILE *fp;

	service = nvram_get("rc_service");

	if(!service)
		kill(1, SIGHUP);

	if(strstr(service,"wan_disconnect")!=NULL)
	{
		cprintf("wan disconnect\n");

		logmessage("wan", "disconnected manually");

		if (nvram_match("wan0_proto", "dhcp") ||
			nvram_match("wan0_proto", "bigpond"))
		{		
			snprintf(tmp, sizeof(tmp), "/var/run/udhcpc%d.pid", 0);
			kill_pidfile_s(tmp, SIGUSR2);
		}
		else 
		{			
			stop_wan2();
		}
	}
	else if (strstr(service,"wan_connect")!=NULL)
	{
		cprintf("wan connect\n");
		logmessage("wan", "connected manually");
		setup_ethernet(nvram_safe_get("wan_ifname"));

		if (nvram_match("wan0_proto", "dhcp") ||
			nvram_match("wan0_proto", "bigpond"))
		{
			snprintf(tmp, sizeof(tmp), "/var/run/udhcpc%d.pid", 0);
			kill_pidfile_s(tmp, SIGUSR1);
		}
		else 
		{
			// pppoe or ppptp, check if /tmp/ppp exist
			if (nvram_invmatch("wan0_proto", "static") &&
			     (fp=fopen("/tmp/ppp/ip-up", "r"))!=NULL)
			{
				fclose(fp);                                     
			}
			else
			{
				stop_wan();
				sleep(3);
	    			start_wan();
				sleep(2);
			}
			/* trigger connect */
			str = nvram_get("wan0_gateway");
			if (str)
				ping_argv[2] = str;
			_eval(ping_argv, NULL, 0, &pid);
		}
	}
	nvram_unset("rc_service");
	return 0;
}

int hotplug_usb_audio(char *product)
{					
	char *wave_argv[]={"waveserver", NULL};
	pid_t pid;

	if (strlen(product)==0) return 1;
	return _eval(wave_argv, ">/dev/null", 0, &pid);
}

int remove_usb_audio(char *product)
{
	eval("killall", "waveserver");
	return 0;
}

int
start_audio(void)
{
	char *wave_argv[] = {"waveservermain", NULL};
	pid_t pid;

	return _eval(wave_argv, NULL, 0, &pid);
}

int
stop_audio(void)
{
	eval("killall", "waveserver");
	return 0;
}


#endif
