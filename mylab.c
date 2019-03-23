#include <stdint.h>
#include <inttypes.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>

/* Include file added by me. */
#include <rte_udp.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <stdio.h>
#include <assert.h>

#define RX_RING_SIZE 128
#define TX_RING_SIZE 512

#define NUM_MBUFS 8191
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 32

static const struct rte_eth_conf port_conf_default = {
	.rxmode = { .max_rx_pkt_len = ETHER_MAX_LEN }
};

/* basicfwd.c: Basic DPDK skeleton forwarding example. */

/*
 * Initializes a given port using global settings and with the RX buffers
 * coming from the mbuf_pool passed as a parameter.
 */
static inline int
port_init(uint8_t port, struct rte_mempool *mbuf_pool)
{
	struct rte_eth_conf port_conf = port_conf_default;
	const uint16_t rx_rings = 1, tx_rings = 1;
	int retval;
	uint16_t q;

	if (port >= rte_eth_dev_count())
		return -1;

	/* Configure the Ethernet device. */
	retval = rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf);
	if (retval != 0)
		return retval;

	/* Allocate and set up 1 RX queue per Ethernet port. */
	for (q = 0; q < rx_rings; q++) {
		retval = rte_eth_rx_queue_setup(port, q, RX_RING_SIZE,
				rte_eth_dev_socket_id(port), NULL, mbuf_pool);
		if (retval < 0)
			return retval;
	}

	/* Allocate and set up 1 TX queue per Ethernet port. */
	for (q = 0; q < tx_rings; q++) {
		retval = rte_eth_tx_queue_setup(port, q, TX_RING_SIZE,
				rte_eth_dev_socket_id(port), NULL);
		if (retval < 0)
			return retval;
	}

	/* Start the Ethernet port. */
	retval = rte_eth_dev_start(port);
	if (retval < 0)
		return retval;

	/* Display the port MAC address. */
	struct ether_addr addr;
	rte_eth_macaddr_get(port, &addr);
	printf("Port %u MAC: %02" PRIx8 " %02" PRIx8 " %02" PRIx8
			   " %02" PRIx8 " %02" PRIx8 " %02" PRIx8 "\n",
			(unsigned)port,
			addr.addr_bytes[0], addr.addr_bytes[1],
			addr.addr_bytes[2], addr.addr_bytes[3],
			addr.addr_bytes[4], addr.addr_bytes[5]);

	/* Enable RX in promiscuous mode for the Ethernet device. */
	rte_eth_promiscuous_enable(port);

	return 0;
}

/*
 * The lcore main. This is the main thread that does the work, reading from
 * an input port and writing to an output port.
 */
static __attribute__((noreturn)) void
lcore_main(struct rte_mempool *mp)
{
	const uint8_t nb_ports = rte_eth_dev_count();
	uint8_t port;

	/*
	 * Check that the port is on the same NUMA node as the polling thread
	 * for best performance.
	 */
	for (port = 0; port < nb_ports; port++)
		if (rte_eth_dev_socket_id(port) > 0 &&
				rte_eth_dev_socket_id(port) !=
						(int)rte_socket_id())
			printf("WARNING, port %u is on remote NUMA node to "
					"polling thread.\n\tPerformance will "
					"not be optimal.\n", port);

	printf("\nCore %u forwarding packets. [Ctrl+C to quit]\n",
			rte_lcore_id());

	/* Run until the application is quit or killed. */
	for (;;) {
		/*
		 * Receive packets on a port and forward them on the paired
		 * port. The mapping is 0 -> 1, 1 -> 0, 2 -> 3, 3 -> 2, etc.
		 */
		for (port = 0; port < nb_ports; port++) {

			/* Get burst of RX packets, from first port of pair. */
			struct rte_mbuf *bufs[1];
			// const uint16_t nb_rx = rte_eth_rx_burst(port, 0,
			// 		bufs, 1);

			// if (unlikely(nb_rx == 0))
			// 	continue;

				// sprintf(rte_pktmbuf_mtod(p_mbuf, char *),
				// 	"Port %u MAC: %02" PRIx8 " %02" PRIx8 " %02" PRIx8 " %02"
				// 	PRIx8 " %02" PRIx8 " %02" PRIx8 "\n",
				// 	(unsigned)port,
				// 	p_ether->s_addr.addr_bytes[0], p_ether->s_addr.addr_bytes[1],
				// 	p_ether->s_addr.addr_bytes[2], p_ether->s_addr.addr_bytes[3],
				// 	p_ether->s_addr.addr_bytes[4], p_ether->s_addr.addr_bytes[5]);

			/* Construct packets and store them into mbufs. */
			for (int i = 0; i < 1; i++) {
				struct rte_mbuf *p_mbuf = rte_pktmbuf_alloc(mp);
				assert(p_mbuf);

				/* Set content of data part. */
				const char *content = "hello world";
				uint16_t content_len = 11;
				rte_pktmbuf_append(p_mbuf, content_len);
				memcpy(rte_pktmbuf_mtod(p_mbuf, void *), content, content_len);

				struct udp_hdr *p_udp = (struct udp_hdr *)rte_pktmbuf_prepend(p_mbuf, sizeof(struct udp_hdr));
				assert(p_udp);
				struct ipv4_hdr *p_ipv4 = (struct ipv4_hdr *)rte_pktmbuf_prepend(p_mbuf, sizeof(struct ipv4_hdr));
				assert(p_ipv4);
				struct ether_hdr *p_ether = (struct ether_hdr *)rte_pktmbuf_prepend(p_mbuf, sizeof(struct ether_hdr));
				assert(p_ether);
				memset(p_udp, 0, sizeof(struct udp_hdr));
				memset(p_ipv4, 0, sizeof(struct ipv4_hdr));
				memset(p_ether, 0, sizeof(struct ether_hdr));

				/* Set udp_hdr. */
				p_udp->src_port = rte_cpu_to_be_16(200);
				p_udp->dst_port = rte_cpu_to_be_16(200);
				p_udp->dgram_len = rte_cpu_to_be_16(sizeof(struct udp_hdr)
					+ content_len);

				/* Set ipv4_hdr. */
				p_ipv4->version_ihl = 0x45;
				p_ipv4->type_of_service = 0;
				p_ipv4->total_length = rte_cpu_to_be_16(sizeof(struct ipv4_hdr)
					+ sizeof(struct udp_hdr) + content_len);
				p_ipv4->packet_id = 0;
				p_ipv4->fragment_offset = 0;
				p_ipv4->time_to_live = 255;
				p_ipv4->next_proto_id = IPPROTO_UDP;
				p_ipv4->src_addr = rte_cpu_to_be_32(IPv4(192, 168, 3, 1));
				p_ipv4->dst_addr = rte_cpu_to_be_32(IPv4(192, 168, 3, 2));

				p_udp->dgram_cksum = rte_ipv4_udptcp_cksum(p_ipv4, p_udp);
				p_ipv4->hdr_checksum = rte_ipv4_cksum(p_ipv4);

				/* Set ether_hdr. */
				rte_eth_macaddr_get(port, &(p_ether->s_addr));
				p_ether->d_addr.addr_bytes[0] = 0x0A;
				p_ether->d_addr.addr_bytes[1] = 0x00;
				p_ether->d_addr.addr_bytes[2] = 0x27;
				p_ether->d_addr.addr_bytes[3] = 0x00;
				p_ether->d_addr.addr_bytes[4] = 0x00;
				p_ether->d_addr.addr_bytes[5] = 0x13;
				p_ether->ether_type = rte_cpu_to_be_16(ETHER_TYPE_IPv4);

				bufs[i] = p_mbuf;
			}

			/* Send burst of TX packets, to second port of pair. */
			const uint16_t nb_tx = rte_eth_tx_burst(port, 0,
					bufs, 1);

			/* Free any unsent packets. */
			if (unlikely(nb_tx < 1)) {
				uint16_t buf;
				for (buf = nb_tx; buf < 1; buf++)
					rte_pktmbuf_free(bufs[buf]);
			}
		}
	}
}

/*
 * The main function, which does initialization and calls the per-lcore
 * functions.
 */
int
main(int argc, char *argv[])
{
	struct rte_mempool *mbuf_pool;
	unsigned nb_ports;
	uint8_t portid;

	/* Initialize the Environment Abstraction Layer (EAL). */
	int ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");

	argc -= ret;
	argv += ret;

	/* Check that there is an even number of ports to send/receive on. */
	nb_ports = rte_eth_dev_count();
	if (nb_ports < 2 || (nb_ports & 1))
		rte_exit(EXIT_FAILURE, "Error: number of ports must be even\n");

	/* Creates a new mempool in memory to hold the mbufs. */
	mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS * nb_ports,
		MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

	if (mbuf_pool == NULL)
		rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

	/* Initialize all ports. */
	for (portid = 0; portid < nb_ports; portid++)
		if (port_init(portid, mbuf_pool) != 0)
			rte_exit(EXIT_FAILURE, "Cannot init port %"PRIu8 "\n",
					portid);

	if (rte_lcore_count() > 1)
		printf("\nWARNING: Too many lcores enabled. Only 1 used.\n");

	/* Call lcore_main on the master core only. */
	lcore_main(mbuf_pool);

	return 0;
}
