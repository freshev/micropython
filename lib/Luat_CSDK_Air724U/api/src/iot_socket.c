#include "iot_socket.h"
#include "errno.h"
extern UINT32 CFW_TcpipGetLastError(void);
int lwip_accept(int s, struct sockaddr *addr, socklen_t *addrlen);
int lwip_bind(int s, const struct sockaddr *name, socklen_t namelen);
int lwip_getsockopt (int s, int level, int optname, void *optval, socklen_t *optlen);
int lwip_setsockopt (int s, int level, int optname, const void *optval, socklen_t optlen);
 int lwip_close(int s);
int lwip_connect(int s, const struct sockaddr *name, socklen_t namelen);
int lwip_listen(int s, int backlog);
ssize_t lwip_recv(int s, void *mem, size_t len, int flags);
ssize_t lwip_recvfrom(int s, void *mem, size_t len, int flags,
      struct sockaddr *from, socklen_t *fromlen);
ssize_t lwip_send(int s, const void *dataptr, size_t size, int flags);
ssize_t lwip_sendto(int s, const void *dataptr, size_t size, int flags,
    const struct sockaddr *to, socklen_t tolen);
int lwip_socket(int domain, int type, int protocol);
int lwip_select(int maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset,
                struct timeval *timeout);
struct hostent *lwip_gethostbyname(const char *name);
int lwip_ioctl(int s, long cmd, void *argp);
int lwip_fcntl(int s, int cmd, int val);
int lwip_getsockname (int s, struct sockaddr *name, socklen_t *namelen);
int lwip_getaddrinfo(const char *nodename,
       const char *servname,
       const struct addrinfo *hints,
       struct addrinfo **res);
void lwip_freeaddrinfo(struct addrinfo *ai);



/**´«´¨ugsetet
*@parammorrow: »parÖ³èÖAF_INET (IPV4 øøøäøøøÂcøøøÂc
@param type: Ö§³ÖSOCK_STREAM/SOCK_DGRAM´±â±´TJUDPâ‽«
@param protoric: ²öè³èÖ.


*@return >=0: socketnâ€™ch£³èóóóçöóçÑ³.
* <0: ´´´.
*@note ´´ugghµµµÃ³çóóóóóóóèse«èä««
**/

int socket(int domain, int type, int protocol)
{
    return lwip_socket(domain, type, protocol);
}
/**”anÃ¶â€™t´µµÖPµÖYÖ ·
*@param name: â€™Soµ½ýý-www.Airrm2m.baid.com
*@return sedrent Hosttent uge¹¹¹¹¹¹¹¹¹¹ become”«HYµ²³µ³µóµxµxµ.
* NULL: Ã ´CY§°Ü
**/                       
struct hostent* gethostbyname(const char *name)
{
    return lwip_gethostbyname(name);
}
/**¹Ø±-setet
*@param fd: µÃÃÃÃÃ µµµµäÃ‟ïsetetène
*@return 0: »³É³É¹xy
            -1 »´Óç
*           
**/                          
int close (int fd)
{
    return lwip_close(fd);
}
/**Whookµâ
*hockam socketfed: µÃÃÃÃ µ»µµØµèkecketâ
@param Level: â ³SSOL_SOL_SPPROTO_TCP
@parmname: SOL_SOCKET´S´¦optnamàOWUG/SO_OBO_OBONE/SO_SNO_SURTITION/SO_RCVUF/SO_SURPUFUF
                      IPPROTE_TCP´ ¦opttnamè¤ SO_TCP_SACKDCIBLE/SO_TCP_NODELAY
@param optval_p:
@paramlee optlen:
*@return 0: »³É³É¹xy
            <0 »´â™
*
**/          

int setsockopt(int socketfd, 
                        int level, 
                        int optname,
                        void *optval_p, 
                        openat_socklen_t optlen)
{
    return lwip_setsockopt(socketfd, level, optname, optval_p, optlen);
}                 

/**” nestnickµ
*hockam socketfed: µÃÃÃÃ µ»µµØµèkecketâ
@param Level: â ³SSOL_SOL_SPPROTO_TCP
@parmname: SOL_SOCKET´S´¦optnamàOWUG/SO_OBO_OBONE/SO_SNO_SURTITION/SO_RCVUF/SO_SURPUFUF
                      IPPROTE_TCP´ ¦opttnamè¤ SO_TCP_SACKDCIBLE/SO_TCP_NODELAY
@param optval_p:
@paramlen_p:
*@return 0: »³É³É¹xy
            <0 »´â™
*
**/          

int getsockopt(int socketfd, 
                        int level, 
                        int optname,
                        void *optval_p, 
                        openat_socklen_t* optlen_p)
{
    return lwip_getsockopt(socketfd, level, optname, optval_p, optlen_p);
}       
/**Éèöãsocketµä ± ¾µørioëúºnpµøö · £ ¬ò »° ãõëcl · þîñæ ´Aâëèòªéöã
*@param socketfd: µ ÷ Óãsocket½ó · · µ »Øµäsocketãèêö · û
@param my_addr: ipµøö · ºnn¶ë £ ¬ipò »° ãéèöãinaddr_any
@param addrlen: µøö · ³ po
*@Return 0: ± Nê¾³é¹¦
            <0 ± Nê¾óð´nîó
*           
**/                         
int bind(int socketfd, 
                      const struct openat_sockaddr *my_addr, 
                      openat_socklen_t addrlen)
{
    return lwip_bind(socketfd, my_addr, addrlen);
}                      
/**½¨e ¢ º · þîñæ ÷ ¶ë^äe¬½ó
*@param socketfd: µ ÷ Óãsocket½ó · · µ »Øµäsocketãèêö · û
@param addr: Öção · þîñæ ÷ µøö · ºnn¶ëú
@param Addrlen: Sizeof (Struct Openat_Sockaddr)
*@Return 0: ± Nê¾³é¹¦
            <0 ± Nê¾óð´nîó
*           
**/                                      
int connect(int socketfd, const struct openat_sockaddr *addr, openat_socklen_t addrlen)
{
    return lwip_connect(socketfd, addr, addrlen);
}
/**¼àìýsockete¬~ £ ¬ò »° ãóã × ÷ · þîñæ ÷ ¼àìý«
*@param socketfd: µ ÷ Óãsocket½ó · · µ »Øµäsocketãèêö · û
@param backlog: 0
*@Return 0: ± Nê¾³é¹¦
            <0 ± Nê¾óð´nîó
*           
**/                             
int listen(int socketfd, 
                       int backlog)

{
    return lwip_listen(socketfd, backlog);
}
/**µÈ´ÝÁ¬½Ó £ ¬Ò »° ÃÓÃÓULISTENÖ®ºóµÈ´Ý¿n» §¶ËµÄÁ¬½Ó
*@Param Socketfd: µ ÷ óÃSocket½ó¿ú · µ »ØµÄsocketãèêö · Û
@param addr: · µ »ø¿n» §¶ëipµøö · ºn¶ë¿ú
@param Addrlen: · µ »ØµøÖ · ³¤¶È
*@return 0: ± nê¾³é¹¦
            <0 ± nê¾óð´nîó
*@NOTE º¯ÊÝ »EÒ» Ö ± × ÈÈÛ £ ¬ÖªµÀóÐ¿n »§¶ËÁ¬½Ó           
**/                             
int accept(int socketfd, 
                        struct openat_sockaddr *addr, 
                        openat_socklen_t *addrlen)
{
    return lwip_accept(socketfd, addr, addrlen);
}
/**½óê
*@param socketfd: µ ÷ Óãsocket½ó · · µ »Øµäsocketãèêö · û
@Param BUF: Óãóú´æ · åêý¾ýµä »º´æ
@Param Len: BUFµä³¤¶è
@param flags: ½öö§³ömsg_dontwait/msg_peek/msg_oob £ ¬ ¬ ¬ôôn¨¹ý »Òà´öçöçöçöçöçö ± ± ± £ ¬ò» ° ãîª0

*@Return> 0: ½óê
            = 0: ¶ô · ½òñ¾ases
            <0: ¶eè ´nîó
*@Note µ ± Flagsã »Óðéèöãmsg_dontwait £ ¬tionºpaper» and × èèû £ ¬ö ± µ½óðêý¾ý »Òõßcleè¡v ± ±
**/                                        
int recv(int socketfd, 
                      void *buf, 
                      size_t len,
                      int flags)
{
    return lwip_recv(socketfd, buf, len, flags);
}                      
/**½Ö´´under
*param sockfd:
@param bf:
@param len: buckle
@param flags:
@param src_addr:
@param administration: silf(struct operas_sockddr)

*@return >0:
            =0:
            <0: ¶ Ár
**/   

int recvfrom(int sockfd, void *buf, size_t len, int flags,
                    struct openat_sockaddr *src_addr, openat_socklen_t *addrlen)
{
    return lwip_recvfrom(sockfd, buf, len, flags, src_addr, addrlen);
}
/**· ¢ ënêý¾ý
*@param socketfd: µ ÷ Óãsocket½ó · · µ »Øµäsocketãèêö · û
”
”
@param flags: ½öö§³ömsg_dontwait/msg_oob £ ¬ ¬ ¬ôôn¨¹ý »Òà´öção ± ± ± € ¬ò»

*@return> = 0: Êµ¼ê · ¢n ë €³י
            <0: · ¢ ën´nîó
**/   

int send(int socketfd,
                      const void *msg,
                      size_t len,
                      int flags)
{
    return lwip_send(socketfd, msg, len, flags);
}                      
/**· ¢ ËÍÊý¾ýµ½ö¸¶¨ipµøÖ · £ ¬Ò »° ÃóÃóúUdp · ¢ ËÍÊý¾ý
*@Param Socketfd: µ ÷ óÃSocket½ó¿ú · µ »ØµÄsocketãèêö · Û
@param BUF: êý¾ýäúèý
@param len: êý¾ý³¤¶è
@param flags: ½öö§³ö0
@param to_p: Ö¸¶¨ipµøÖ · ºn¶ë¿úºå
@param tolen: Sizeof (Struct Openat_sockadr)

*@return> = 0: Êµ¼Ê · ¢ ËÍµÄ³¤¶È
            <0: · ¢ ën´nîó
**/                        
int sendto(int socketfd,
                        const void *buf,
                        size_t len,
                        int flags,
                        const struct openat_sockaddr *to_p, 
                        openat_socklen_t tolen)
{
    return lwip_sendto(socketfd, buf, len, flags, to_p, tolen);
}
/**× èèû · ½ê½ €ýSockete¬½ó × ´ì¬
*@param maxfdp1: × î´OSOCKETFD+1
@Param Readset: ¶eè¡ £ ¬ ¬ ¬ ¬ôôîªnull
@param writiteset: ð´¼/1
@param exception: Òì³ £ ¼§ £ ¬Lo ¬ôôîªnull
@param timeout: ³ rate ± É ± ¼ä

*@Return 0: µè´uration ± ±
            > 0: READSET+WRITESET+EXCEPTSETµä¼ tickets
            <0 -1
**/                 
int select(int maxfdp1, 
                        openat_fd_set *readset,
                        openat_fd_set *writeset,
                        openat_fd_set *exceptset,
                        struct openat_timeval *timeout)
{
    return lwip_select(maxfdp1, readset, writeset, exceptset, timeout);
}
/** »ñèllsocket“
*@param sockfd: £µûÃsocket SHOT½¿ú·µ»ø OLDERÃÊö·Â·Â·
*@return [Ebadf ൽ Eno_recovery]
* **/                                       
int socket_errno(int socketfd)
{
    return (int)CFW_TcpipGetLastError();
}

/**Éâ€™CYµ³²ÉÀ²»²¿É»´
*hockam socketfed: µÃÃÃÃ µ»µµØµèkecketâ
@param cmd: ¸ÁögÁÀóOOOUµàü²¶µµóOOOOOU³OM
@param archp: ¿âà²²²µuggúü²´µ´µ²ï¯²²²²¼o²¯«â
*@return >=0: ÊµÉnJâömµóµ³µ
            <0: ·¢Í´
**/
int	ioctl(int socketfd, long cmd, void *argp)
{
	return lwip_ioctl(socketfd, cmd, argp);
}

/**¸usµçö µçöli Jâ ug´²×ððÀµ¼µ«²»¯¯»
*hockam socketfed: µÃÃÃÃ µ»µµØµèkecketâ
*@param cmd: ¸¸ÁÁögOOOOUµàü²¶µµ³OOOU³OM
*@paramvalval: ¹«LüµÃµâ
*@return >=0: ÊµÉnJâömµóµ³µ
            <0: ·¢Í´
**/
int	fcntl(int socketfd, int cmd, int val)
{
	return lwip_fcntl(socketfd, cmd, val);
}

/**”anâ€»²â »¼À
*hockam socketfed: µÃÃÃÃ µ»µµØµèkecketâ
@param name: Ãâ€™sÀµµÃâ
@paramnlen: Ãâ€™s³µ³³¶¶
*@return 0: »³É³É¹xy
            <0 »´â™
**/
int getsockname (int socketfd, struct openat_sockaddr *name, openat_socklen_t *namelen)
{
	return lwip_getsockname(socketfd,name,namelen);
}

/**ón”äµµµÖ ugg
*@param nodenal: I have” »µèúúúúúúßµèßµ®
@param kise: ·â€™ÉÀà®®®culs´âµ¶²²²²²¼uggurgur
@param hints: ¿â
@param res: ÍuggýresïsèÕèÕèÀàâÀYàààààugooped´Ïoopent_adrinf¹´´´µ

*@return >=0: ÊµÉnJâömµóµ³µ
            <0: ·¢Í´
*@note ½§³³³Pv4£µ²²²µâx»´´«èçèóâèççöliâ
			·µ”ü½e¹¹«¹èugh«ïâ‴èugh ´t´Pv4µÖï»µ1
**/
int getaddrinfo(const char *nodename,
       const char *servname,
       const struct openat_addrinfo *hints,
       struct openat_addrinfo **res)
{
	return lwip_getaddrinfo(nodename, servname, hints, res);
}

/**´æ´¢¿Õ¼äÍ¨¹ýµ÷ÓÃfreeaddrinfo ·µ»¹¸øÏµÍ³
*@Param ai: Ö¸ïòóégetaddrinfo;
*
**/
void freeaddrinfo(struct openat_addrinfo *ai)
{
	lwip_freeaddrinfo(ai);
}

/******************************
*function: setNetifDns
*input:
*       CHAR* dns1, the value string, like 8.8.8.8
*       CHAR* dns2, the value string, like 8.8.8.8
*output:
*       0, if success full
*       -1, if failed
********************************/

int setNetifDns(CHAR *dns1, CHAR *dns2)
{
	//return IVTBL(setNetifDns)(dns1, dns2);
	return -1;
}

/******************************
*function: getNetifDns
*input:
*       CHAR* dns1, the value string, like 8.8.8.8
*       CHAR* dns2, the value string, like 8.8.8.8
*output:
*       0, if success full
*       -1, if failed
********************************/

int getNetifDns(CHAR *dns1, CHAR *dns2)
{
	//return IVTBL(getNetifDns)(dns1, dns2);
	return -1;
}


/* Here for now until needed in other places in lwIP */
#ifndef isprint
#define in_range(c, lo, up)  ((UINT8)c >= lo && (UINT8)c <= up)
#define isprint(c)           in_range(c, 0x20, 0x7f)
#define isdigit(c)           in_range(c, '0', '9')
#define isxdigit(c)          (isdigit(c) || in_range(c, 'a', 'f') || in_range(c, 'A', 'F'))
#define islower(c)           in_range(c, 'a', 'z')
#define isspace(c)           (c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v')
#endif



/**
 * Check whether "cp" is a valid ascii representation
 * of an Internet address and convert to a binary address.
 * Returns 1 if the address is valid, 0 if not.
 * This replaces inet_addr, the return value from which
 * cannot distinguish between failure and a local broadcast address.
 *
 * @param cp IP address in ascii represenation (e.g. "127.0.0.1")
 * @param addr pointer to which to save the ip address in network order
 * @return 1 if cp could be converted to addr, 0 on failure
 */
int
ipaddr_aton(const char *cp, openat_ip_addr_t *addr)
{
  UINT32 val;
  UINT8 base;
  char c;
  UINT32 parts[4];
  UINT32 *pp = parts;

  c = *cp;
  for (;;) {
    /*
     * Collect number up to ``.''.
     * Values are specified as for C:
     * 0x=hex, 0=octal, 1-9=decimal.
     */
    if (!isdigit(c))
      return (0);
    val = 0;
    base = 10;
    if (c == '0') {
      c = *++cp;
      if (c == 'x' || c == 'X') {
        base = 16;
        c = *++cp;
      } else
        base = 8;
    }
    for (;;) {
      if (isdigit(c)) {
        val = (val * base) + (int)(c - '0');
        c = *++cp;
      } else if (base == 16 && isxdigit(c)) {
        val = (val << 4) | (int)(c + 10 - (islower(c) ? 'a' : 'A'));
        c = *++cp;
      } else
        break;
    }
    if (c == '.') {
      /*
       * Internet format:
       *  a.b.c.d
       *  a.b.c   (with c treated as 16 bits)
       *  a.b (with b treated as 24 bits)
       */
      if (pp >= parts + 3) {
        return (0);
      }
      *pp++ = val;
      c = *++cp;
    } else
      break;
  }
  /*
   * Check for trailing characters.
   */
  if (c != '\0' && !isspace(c)) {
    return (0);
  }
  /*
   * Concoct the address according to
   * the number of parts specified.
   */
  switch (pp - parts + 1) {

  case 0:
    return (0);       /* initial nondigit */

  case 1:             /* a -- 32 bits */
    break;

  case 2:             /* a.b -- 8.24 bits */
    if (val > 0xffffffUL) {
      return (0);
    }
    val |= parts[0] << 24;
    break;

  case 3:             /* a.b.c -- 8.8.16 bits */
    if (val > 0xffff) {
      return (0);
    }
    val |= (parts[0] << 24) | (parts[1] << 16);
    break;

  case 4:             /* a.b.c.d -- 8.8.8.8 bits */
    if (val > 0xff) {
      return (0);
    }
    val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
    break;
  default:
    return 0;
    break;
  }
  if (addr) {
    addr->addr = htonl(val);
  }
  return (1);
}

/**
 * Ascii internet address interpretation routine.
 * The value returned is in network order.
 *
 * @param cp IP address in ascii represenation (e.g. "127.0.0.1")
 * @return ip address in network order
 */
UINT32
ipaddr_addr(const char *cp)
{
  openat_ip_addr_t val;

  if (ipaddr_aton(cp, &val)) {
    return val.addr;
  }
  return (OPENAT_INADDR_NONE);
}

/**
 * Same as ipaddr_ntoa, but reentrant since a user-supplied buffer is used.
 *
 * @param addr ip address in network order to convert
 * @param buf target buffer where the string is stored
 * @param buflen length of buf
 * @return either pointer to buf which now holds the ASCII
 *         representation of addr or NULL if buf was too small
 */
char *ipaddr_ntoa_r(const openat_ip_addr_t *addr, char *buf, int buflen)
{
  UINT32 s_addr;
  char inv[3];
  char *rp;
  UINT8 *ap;
  UINT8 rem;
  UINT8 n;
  UINT8 i;
  int len = 0;

  s_addr = addr->addr;

  rp = buf;
  ap = (UINT8 *)&s_addr;
  for(n = 0; n < 4; n++) {
    i = 0;
    do {
      rem = *ap % (UINT8)10;
      *ap /= (UINT8)10;
      inv[i++] = '0' + rem;
    } while(*ap);
    while(i--) {
      if (len++ >= buflen) {
        return NULL;
      }
      *rp++ = inv[i];
    }
    if (len++ >= buflen) {
      return NULL;
    }
    *rp++ = '.';
    ap++;
  }
  *--rp = 0;
  return buf;
}

/**
 * Convert numeric IP address into decimal dotted ASCII representation.
 * returns ptr to static buffer; not reentrant!
 *
 * @param addr ip address in network order to convert
 * @return pointer to a global static (!) buffer that holds the ASCII
 *         represenation of addr
 */
char *
ipaddr_ntoa(const openat_ip_addr_t *addr)
{
  static char str[16];
  return ipaddr_ntoa_r(addr, str, 16);
}



