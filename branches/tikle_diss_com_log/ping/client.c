#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#define BUFLEN 512
#define NPACK 10
#define PORT 9930

#define DKV "10.0.0.1"
#define FUSCA "10.0.0.2"
#define PASSAT "10.0.0.3"
#define PORSCHE "10.0.0.4"
#define SRV_IP "127.0.0.1"

int main(int argc, char *argv[])
{
	int nmsgs = atoi(argv[1]), num;
	struct sockaddr_in si_other;
	int s, i=0, j=0, media, slen=sizeof(si_other);
	char buf[BUFLEN];

	if (nmsgs < 60) nmsgs = 60;
	media = nmsgs/60;

	printf("Enviando %d mensagens em 60 segundos\n", nmsgs);
	printf("%d por segundo\n", media);

	s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	memset((char *) &si_other, 0, sizeof(si_other));
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(PORT);
	if (inet_aton(SRV_IP, &si_other.sin_addr)==0) {
		fprintf(stderr, "inet_aton() failed\n");
		exit(1);
	}

	for (i=0; i<60; i++) {
		for (j=0;j<media;j++) {
			srand(time(NULL));
			num = rand() % 4;
			if (strcmp("dkv", argv[2]) == 0) {
				if (num == 0)
					inet_aton(FUSCA, &si_other.sin_addr);
				else if (num == 1)
					inet_aton(PASSAT, &si_other.sin_addr);
				else
					inet_aton(PORSCHE, &si_other.sin_addr);
			} else if (strcmp("fusca", argv[2]) == 0) {
				if (num == 0)
					inet_aton(DKV, &si_other.sin_addr);
				else if (num == 1)
					inet_aton(PASSAT, &si_other.sin_addr);
				else
					inet_aton(PORSCHE, &si_other.sin_addr);

			} else if (strcmp("passat", argv[2]) == 0) {
				if (num == 0)
					inet_aton(DKV, &si_other.sin_addr);
				else if (num == 1)
					inet_aton(FUSCA, &si_other.sin_addr);
				else
					inet_aton(PORSCHE, &si_other.sin_addr);

			} else if (strcmp("porsche", argv[2]) == 0) {
				if (num == 0)
					inet_aton(DKV, &si_other.sin_addr);
				else if (num == 1)
					inet_aton(FUSCA, &si_other.sin_addr);
				else
					inet_aton(PASSAT, &si_other.sin_addr);

			}
			sendto(s, buf, BUFLEN, 0, &si_other, slen);
		}
		sleep(1);
	}
	close(s);
	return 0;
}
