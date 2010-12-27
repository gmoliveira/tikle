#include <stdio.h>
#include <pcap.h>

char *f_get_default_device(void)
{
	char *device = NULL;
	char errbuf[PCAP_ERRBUF_SIZE];

	device = pcap_lookupdev(errbuf);

	if (!device) {
		fprintf(stderr, "Couldn't find default device: %s\n", errbuf);
		return device;
	}

	return device;
}
