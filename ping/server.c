#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>

#define BUFLEN 512
#define NPACK 10
#define PORT 9930

int main(int argc, char *argv[])
{
	int nmsgs = atoi(argv[1]);
	struct sockaddr_in si_me, si_other;
	int s, i=0, j=0, media, slen=sizeof(si_other);
	char buf[BUFLEN];

	if (nmsgs < 60) nmsgs = 60;
	media = nmsgs/60;

	printf("Recebendo %d mensagens em 60 segundos\n", nmsgs);
	printf("%d por segundo\n", media);

	s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	memset((char *) &si_me, 0, sizeof(si_me));
	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(PORT);
	si_me.sin_addr.s_addr = htonl(INADDR_ANY);
	bind(s, &si_me, sizeof(si_me));

	for (i=0; i<60; i++) {
		for (j=0;j<media;j++) {
			recvfrom(s, buf, BUFLEN, 0, &si_other, &slen);
			printf("Mensagem de %s\n", inet_ntoa(si_other.sin_addr));
		}
		sleep(1);
	}
	close(s);
	return 0;
}
