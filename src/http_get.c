/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>

#if !defined(__ZEPHYR__) || defined(CONFIG_POSIX_API)

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

#else

#include <zephyr/net/socket.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/kernel.h>

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
#include <zephyr/net/tls_credentials.h>
#include "ca_certificate.h"
#endif

#endif

#include "wifi_mgr.h"

/* HTTP server to connect to */
#define HTTP_HOST "www.baidu.com"
// #define HTTP_HOST "cdn.iflyos.cn"
/* Port to connect to, as string */
#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
#define HTTP_PORT "443"
#else
#define HTTP_PORT "80"
#endif
/* HTTP path to request */
#define HTTP_PATH "/"
// #define HTTP_PATH "/llm/users/16/images/5df62c6f-f4b1-457e-a2ec-4bf7f33bdd0c.png?x-oss-process=image/resize,w_100,limit_0"


#define SSTRLEN(s) (sizeof(s) - 1)
#define CHECK(r) { if (r == -1) { printf("Error: " #r "\n"); exit(1); } }

#define REQUEST "GET " HTTP_PATH " HTTP/1.0\r\nHost: " HTTP_HOST "\r\n\r\n"

static char response[1024];

void dump_addrinfo(const struct addrinfo *ai)
{
	printf("addrinfo @%p: ai_family=%d, ai_socktype=%d, ai_protocol=%d, "
	       "sa_family=%d, sin_port=%x\n",
	       ai, ai->ai_family, ai->ai_socktype, ai->ai_protocol,
	       ai->ai_addr->sa_family,
	       ((struct sockaddr_in *)ai->ai_addr)->sin_port);
}

int main(void)
{
	static struct addrinfo hints;
	struct addrinfo *res;
	int st, sock;

	printf("hello world\n");
	wifi_init("TreeNewbear", "csk_wifi_connect");
	k_msleep(6000);
	printf("after wifi init\n");
	

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
	tls_credential_add(CA_CERTIFICATE_TAG, TLS_CREDENTIAL_CA_CERTIFICATE,
			   ca_certificate, sizeof(ca_certificate));
#endif

	printf("Preparing HTTP GET request for http://" HTTP_HOST
	       ":" HTTP_PORT HTTP_PATH "\n");

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	st = getaddrinfo(HTTP_HOST, HTTP_PORT, &hints, &res);
	printf("getaddrinfo status: %d\n", st);

	if (st != 0) {
		printf("Unable to resolve address, quitting\n");
		return 0;
	}

#if 0
	for (; res; res = res->ai_next) {
		dump_addrinfo(res);
	}
#endif

	dump_addrinfo(res);

	struct sockaddr_in addr = {0};
	addr.sin_family = AF_INET;
	memcpy(&addr.sin_addr, &((struct sockaddr_in *)(res->ai_addr))->sin_addr, sizeof(addr.sin_addr));

    char buf[NET_IPV4_ADDR_LEN];
    printf("host ip address: %s\r\n", net_addr_ntop(AF_INET, &addr.sin_addr, buf, sizeof(buf)));


#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
	printk("1\r\n");
	printf("1\r\n");
	sock = socket(res->ai_family, res->ai_socktype, IPPROTO_TLS_1_2);
#else
	sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
#endif
	CHECK(sock);
	printf("sock = %d\n", sock);

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
	printk("2\r\n");
	printf("2\r\n");
	sec_tag_t sec_tag_opt[] = {
		CA_CERTIFICATE_TAG,
	};
	CHECK(setsockopt(sock, SOL_TLS, TLS_SEC_TAG_LIST,
			 sec_tag_opt, sizeof(sec_tag_opt)));

	CHECK(setsockopt(sock, SOL_TLS, TLS_HOSTNAME,
			 HTTP_HOST, sizeof(HTTP_HOST)))
#endif

	// CHECK(connect(sock, res->ai_addr, res->ai_addrlen));
	int ret = connect(sock, res->ai_addr, res->ai_addrlen);
	if(ret != 0){
		printf("connect failed: ret: %d\r\n", ret);
	}

	CHECK(send(sock, REQUEST, SSTRLEN(REQUEST), 0));

	printf("Response:\n\n");

	while (1) {
		int len = recv(sock, response, sizeof(response) - 1, 0);

		if (len < 0) {
			printf("Error reading response\n");
			return 0;
		}

		if (len == 0) {
			break;
		}

		response[len] = 0;
		printf("%s", response);
	}

	printf("\n");

	(void)close(sock);
	return 0;
}
