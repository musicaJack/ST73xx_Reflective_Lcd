#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H

// lwIP选项配置 - 基于AsyncDNSServer_RP2040W项目优化

#define NO_SYS                      1
#define LWIP_SOCKET                 0
#define LWIP_NETCONN                0

// 协议配置
#define LWIP_IPV4                   1
#define LWIP_IPV6                   0
#define LWIP_UDP                    1
#define LWIP_TCP                    1
#define LWIP_ICMP                   1
#define LWIP_RAW                    1

// DHCP配置
#define LWIP_DHCP                   1
#define LWIP_AUTOIP                 0
#define LWIP_DHCP_DOES_ACD_CHECK    0

// DNS配置
#define LWIP_DNS                    1
#define DNS_TABLE_SIZE              4
#define DNS_MAX_NAME_LENGTH         256

// 网络接口配置
#define LWIP_NETIF_HOSTNAME         1
#define LWIP_NETIF_STATUS_CALLBACK  1
#define LWIP_NETIF_LINK_CALLBACK    1

// 简化的内存配置
#define MEM_LIBC_MALLOC             0
#define MEMP_MEM_MALLOC             0
#define MEM_ALIGNMENT               4
#define MEM_SIZE                    4000

// PBUF配置
#define PBUF_POOL_SIZE              24
#define PBUF_POOL_BUFSIZE           1524

// ARP配置
#define LWIP_ARP                    1
#define ARP_TABLE_SIZE              10
#define ARP_QUEUEING                0

// IP配置
#define IP_FORWARD                  0
#define IP_OPTIONS_ALLOWED          1
#define IP_REASSEMBLY               1
#define IP_FRAG                     1
#define IP_REASS_MAXAGE             15
#define IP_REASS_MAX_PBUFS          10
#define IP_DEFAULT_TTL              255

// ICMP配置
#define LWIP_ICMP                   1
#define ICMP_TTL                    (IP_DEFAULT_TTL)
#define LWIP_BROADCAST_PING         0
#define LWIP_MULTICAST_PING         0

// TCP配置
#define TCP_TTL                     (IP_DEFAULT_TTL)
#define TCP_WND                     (4 * TCP_MSS)
#define TCP_MAXRTX                  12
#define TCP_SYNMAXRTX               6
#define TCP_QUEUE_OOSEQ             0
#define TCP_MSS                     1024
#define TCP_SND_BUF                 (4 * TCP_MSS)
#define TCP_OVERSIZE                TCP_MSS
#define LWIP_TCP_TIMESTAMPS         0

// UDP配置
#define UDP_TTL                     (IP_DEFAULT_TTL)
#define LWIP_UDPLITE                0

// 统计和调试
#define LWIP_STATS                  0
#define LWIP_PROVIDE_ERRNO          1
#define LWIP_DEBUG                  0

// 多播
#define LWIP_IGMP                   0

// 随机数生成
#ifndef LWIP_RAND
#define LWIP_RAND() ((u32_t)rand())
#endif

#endif // _LWIPOPTS_H