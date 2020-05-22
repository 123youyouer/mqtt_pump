//
// Created by root on 2020/3/18.
//

#ifndef PUMP_INIT_HH
#define PUMP_INIT_HH

#include <string>
#include <dpdk/include/rte_timer.h>
#include <common/cpu.hh>
#include <engine/dpdk/freebsd_init.hh>

namespace engine::dpdk{
    static struct rte_timer freebsd_clock;

    // Mellanox Linux's driver key
    static uint8_t default_rsskey_40bytes[40] = {
            0xd1, 0x81, 0xc6, 0x2c, 0xf7, 0xf4, 0xdb, 0x5b,
            0x19, 0x83, 0xa2, 0xfc, 0x94, 0x3e, 0x1a, 0xdb,
            0xd9, 0x38, 0x9e, 0x6b, 0xd1, 0x03, 0x9c, 0x2c,
            0xa7, 0x44, 0x99, 0xad, 0x59, 0x3d, 0x56, 0xd9,
            0xf3, 0x25, 0x3c, 0x06, 0x2a, 0xdc, 0x1f, 0xfc
    };

    static int use_rsskey_52bytes = 0;
    static uint8_t default_rsskey_52bytes[52] = {
            0x44, 0x39, 0x79, 0x6b, 0xb5, 0x4c, 0x50, 0x23,
            0xb6, 0x75, 0xea, 0x5b, 0x12, 0x4f, 0x9f, 0x30,
            0xb8, 0xa2, 0xc0, 0x3d, 0xdf, 0xdc, 0x4d, 0x02,
            0xa0, 0x8c, 0x9b, 0x33, 0x4a, 0xf6, 0x4a, 0x4c,
            0x05, 0xc6, 0xfa, 0x34, 0x39, 0x58, 0xd8, 0x55,
            0x7d, 0x99, 0x58, 0x3a, 0xe1, 0x38, 0xc9, 0x2e,
            0x81, 0x15, 0x03, 0x66
    };
    struct hw_features_data {
        uint8_t rx_csum;
        uint8_t rx_lro;
        uint8_t tx_csum_ip;
        uint8_t tx_csum_l4;
        uint8_t tx_tso;
    };
    struct port_cfg {
        uint16_t rss_reta_size;
        hw_features_data hw_features;
        uint8_t mac[6];
        std::string addr;
        std::string netmask;
        std::string broadcast;
        std::string gateway;
    };
    struct dpdk_config {
        std::string proc_type;
        std::string proc_id;
        std::string memory;
        std::string lcore_mask;
        std::string base_virtaddr;
        int nb_channel;
        int nb_procs;
        int promiscuous;
        int numa_on;
        int tso;
        int tx_csum_offoad_skip;
        unsigned idle_sleep;
        unsigned pkt_tx_delay;
        std::vector<port_cfg> port_config;
    };
    dpdk_config glb_dpdk_config;
    rte_mempool *pktmbuf_pool[8];

    int set_dpdk_argv(char** argv){
        int n=0;
        char temp[256] = {0}, temp2[256] = {0};
        if(!glb_dpdk_config.proc_type.empty()){
            std::sprintf(temp,"--proc_type=%s",glb_dpdk_config.proc_type.c_str());
            argv[n++]=strdup(temp);
        }
        if(!glb_dpdk_config.proc_id.empty()){
            std::sprintf(temp,"-c%s",glb_dpdk_config.proc_id.c_str());
            argv[n++]=strdup(temp);
        }
        if(!glb_dpdk_config.memory.empty()){
            std::sprintf(temp,"-m%s",glb_dpdk_config.memory.c_str());
            argv[n++]=strdup(temp);
        }
        if(!glb_dpdk_config.nb_channel>0){
            std::sprintf(temp,"-n%d",glb_dpdk_config.nb_channel);
            argv[n++]=strdup(temp);
        }
        if(!glb_dpdk_config.base_virtaddr.empty()){
            std::sprintf(temp,"--base-virtaddr=%s",glb_dpdk_config.base_virtaddr.c_str());
            argv[n++]=strdup(temp);
        }

        return n;
    }
    void init_memory_buf(){
        for(int i=0;i<engine::hw::the_cpu_count;++i){
            uint32_t sid=rte_lcore_to_socket_id(i);
            if (pktmbuf_pool[sid] != nullptr)
                continue;
            RTE_ALIGN_CEIL(8192*4,8192);
            std::string name="pktmbuf_pool_";
            name+=std::to_string(sid);
            pktmbuf_pool[sid]=rte_pktmbuf_pool_create(name.c_str(),RTE_ALIGN_CEIL(8192*4,8192),256,0,2048+128,sid);
        }
    }
    static void
    set_rss_table(uint16_t port_id, uint16_t reta_size, uint16_t nb_queues)
    {
        if (reta_size == 0) {
            return;
        }

        int reta_conf_size = RTE_MAX(1, reta_size / RTE_RETA_GROUP_SIZE);
        struct rte_eth_rss_reta_entry64 reta_conf[reta_conf_size];

        /* config HW indirection table */
        unsigned i, j, hash=0;
        for (i = 0; i < reta_conf_size; i++) {
            reta_conf[i].mask = ~0ULL;
            for (j = 0; j < RTE_RETA_GROUP_SIZE; j++) {
                reta_conf[i].reta[j] = hash++ % nb_queues;
            }
        }

        if (rte_eth_dev_rss_reta_update(port_id, reta_conf, reta_size)) {
            rte_exit(EXIT_FAILURE, "port[%d], failed to update rss table\n",
                     port_id);
        }
    }
    static void
    check_all_ports_link_status()
    {
        int max_check_turn=9;
        int check_interval=100;
        uint16_t portid;
        uint8_t count, all_ports_up, print_flag = 0;
        struct rte_eth_link link;

        printf("\nChecking link status");
        fflush(stdout);

        for (count = 0; count <= max_check_turn; count++) {
            all_ports_up = 1;
            for (int i = 0; i < glb_dpdk_config.port_config.size(); i++) {
                uint16_t portid = i;
                memset(&link, 0, sizeof(link));
                rte_eth_link_get_nowait(portid, &link);

                /* print link status if flag set */
                if (print_flag == 1) {
                    if (link.link_status) {
                        printf("Port %d Link Up - speed %u "
                               "Mbps - %s\n", (int)portid,
                               (unsigned)link.link_speed,
                               (link.link_duplex == ETH_LINK_FULL_DUPLEX) ?
                               ("full-duplex") : ("half-duplex\n"));
                    } else {
                        printf("Port %d Link Down\n", (int)portid);
                    }
                    continue;
                }
                /* clear all_ports_up flag if any link down */
                if (link.link_status == 0) {
                    all_ports_up = 0;
                    break;
                }
            }

            /* after finally printing all link status, get out */
            if (print_flag == 1)
                break;

            if (all_ports_up == 0) {
                printf(".");
                fflush(stdout);
                rte_delay_ms(check_interval);
            }

            /* set the print_flag if all ports up or timeout */
            if (all_ports_up == 1 || count == (max_check_turn - 1)) {
                print_flag = 1;
                printf("done\n");
            }
        }
    }

    void init_port(){
        for(int i=0;i<glb_dpdk_config.port_config.size();++i){
            rte_eth_dev_info dev_info = {0};
            uint16_t port_id=i;

            rte_eth_dev_info_get(port_id, &dev_info);
            uint8_t n_procs=std::min(engine::hw::the_cpu_count,std::min(dev_info.max_tx_queues,dev_info.max_rx_queues));
            ether_addr addr = {0};
            rte_eth_macaddr_get(port_id, &addr);
            memcpy(glb_dpdk_config.port_config[i].mac,addr.addr_bytes,ETHER_ADDR_LEN);

            uint64_t default_rss_hf = ETH_RSS_PROTO_MASK;
            struct rte_eth_conf port_conf = {0};
            port_conf.rxmode.mq_mode = ETH_MQ_RX_RSS;
            port_conf.rx_adv_conf.rss_conf.rss_hf = default_rss_hf;
            switch (dev_info.hash_key_size){
                case 52:
                    port_conf.rx_adv_conf.rss_conf.rss_key = default_rsskey_52bytes;
                    port_conf.rx_adv_conf.rss_conf.rss_key_len = 52;
                    use_rsskey_52bytes = 1;
                    break;
                default:
                    port_conf.rx_adv_conf.rss_conf.rss_key = default_rsskey_40bytes;
                    port_conf.rx_adv_conf.rss_conf.rss_key_len = 40;
                    break;
            }

            port_conf.rx_adv_conf.rss_conf.rss_hf &= dev_info.flow_type_rss_offloads;
            if (port_conf.rx_adv_conf.rss_conf.rss_hf != ETH_RSS_PROTO_MASK)
                rte_log(RTE_LOG_EMERG,RTE_LOGTYPE_USER1,"Port %u modified RSS hash function based on hardware support,",port_id);

            if (dev_info.tx_offload_capa & DEV_TX_OFFLOAD_MBUF_FAST_FREE)
                port_conf.txmode.offloads |= DEV_TX_OFFLOAD_MBUF_FAST_FREE;

            port_conf.rxmode.offloads &= ~DEV_RX_OFFLOAD_KEEP_CRC;

            if ((dev_info.rx_offload_capa & DEV_RX_OFFLOAD_IPV4_CKSUM) &&
                (dev_info.rx_offload_capa & DEV_RX_OFFLOAD_UDP_CKSUM) &&
                (dev_info.rx_offload_capa & DEV_RX_OFFLOAD_TCP_CKSUM)) {
                printf("RX checksum offload supported\n");
                port_conf.rxmode.offloads |= DEV_RX_OFFLOAD_CHECKSUM;
                glb_dpdk_config.port_config[i].hw_features.rx_csum = 1;
            }

            if (ff_global_cfg.dpdk.tx_csum_offoad_skip == 0) {
                if ((dev_info.tx_offload_capa & DEV_TX_OFFLOAD_IPV4_CKSUM)) {
                    printf("TX ip checksum offload supported\n");
                    port_conf.txmode.offloads |= DEV_TX_OFFLOAD_IPV4_CKSUM;
                    glb_dpdk_config.port_config[i].hw_features.tx_csum_ip = 1;
                }

                if ((dev_info.tx_offload_capa & DEV_TX_OFFLOAD_UDP_CKSUM) &&
                    (dev_info.tx_offload_capa & DEV_TX_OFFLOAD_TCP_CKSUM)) {
                    printf("TX TCP&UDP checksum offload supported\n");
                    port_conf.txmode.offloads |= DEV_TX_OFFLOAD_UDP_CKSUM | DEV_TX_OFFLOAD_TCP_CKSUM;
                    glb_dpdk_config.port_config[i].hw_features.tx_csum_l4 = 1;
                }
            } else {
                printf("TX checksum offoad is disabled\n");
            }

            if (ff_global_cfg.dpdk.tso) {
                if (dev_info.tx_offload_capa & DEV_TX_OFFLOAD_TCP_TSO) {
                    printf("TSO is supported\n");
                    port_conf.txmode.offloads |= DEV_TX_OFFLOAD_TCP_TSO;
                    glb_dpdk_config.port_config[i].hw_features.tx_tso = 1;
                }
            } else {
                printf("TSO is disabled\n");
            }

            if (dev_info.reta_size) {
                /* reta size must be power of 2 */
                assert((dev_info.reta_size & (dev_info.reta_size - 1)) == 0);

                glb_dpdk_config.port_config[i].rss_reta_size = dev_info.reta_size;
                printf("port[%d]: rss table size: %d\n", port_id,
                       dev_info.reta_size);
            }
            if (rte_eal_process_type() != RTE_PROC_PRIMARY)
                continue;

            if (rte_eth_dev_configure(port_id, n_procs, n_procs, &port_conf))
                rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");

            uint16_t nb_rxd = 512;
            uint16_t nb_txd = 512;
            if (rte_eth_dev_adjust_nb_rx_tx_desc(port_id, &nb_rxd, &nb_txd))
                rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");

            uint16_t q;
            for (q = 0; q < n_procs; q++){
                uint32_t sid=rte_lcore_to_socket_id(q);
                rte_eth_rxconf rxq_conf=dev_info.default_rxconf;
                rte_eth_txconf txq_conf=dev_info.default_txconf;

                txq_conf.offloads = port_conf.txmode.offloads;
                if(rte_eth_tx_queue_setup(port_id, q, nb_txd, sid, &txq_conf)<0)
                    rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");

                rxq_conf.offloads = port_conf.rxmode.offloads;
                if(rte_eth_rx_queue_setup(port_id, q, nb_rxd, sid, &rxq_conf,pktmbuf_pool[sid])<0)
                    rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");
            }

            if(rte_eth_dev_start(port_id)<0)
                rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");

            rte_eth_promiscuous_disable(port_id);

            if (n_procs > 1) {
                /* set HW rss hash function to Toeplitz. */
                if (!rte_eth_dev_filter_supported(port_id, RTE_ETH_FILTER_HASH)) {
                    rte_eth_hash_filter_info info = {};
                    info.info_type = RTE_ETH_HASH_FILTER_GLOBAL_CONFIG;
                    info.info.global_conf.hash_func = RTE_ETH_HASH_FUNCTION_TOEPLITZ;

                    if (rte_eth_dev_filter_ctrl(port_id, RTE_ETH_FILTER_HASH,
                                                RTE_ETH_FILTER_SET, &info) < 0) {
                        rte_exit(EXIT_FAILURE, "port[%d] set hash func failed\n",
                                 port_id);
                    }
                }

                set_rss_table(port_id, dev_info.reta_size, n_procs);
            }

            rte_eth_fc_conf fc_conf={};
            memset(&fc_conf, 0, sizeof(fc_conf));
            if (rte_eth_dev_flow_ctrl_get(port_id, &fc_conf)<0)
                rte_exit(EXIT_FAILURE, "port[%d] set hash func failed\n",
                         port_id);

            /* and just disable the rx/tx flow control */
            fc_conf.mode = RTE_FC_NONE;
            if (rte_eth_dev_flow_ctrl_set(port_id, &fc_conf)<0)
                rte_exit(EXIT_FAILURE, "port[%d] set hash func failed\n",
                         port_id);
        }
        if (rte_eal_process_type() == RTE_PROC_PRIMARY) {
            check_all_ports_link_status();
        }
    };

    static int
    init_clock(void)
    {
        rte_timer_subsystem_init();
        uint64_t hz = rte_get_timer_hz();
        uint64_t intrs = MS_PER_S/ff_global_cfg.freebsd.hz;
        uint64_t tsc = (hz + MS_PER_S - 1) / MS_PER_S*intrs;

        rte_timer_init(&freebsd_clock);
        rte_timer_reset(&freebsd_clock, tsc, PERIODICAL, rte_lcore_id(), [](rte_timer* t, void* a){}, nullptr);

        return 0;
    }

    void init_dpdk(int argc, char **argv){
        if(rte_eal_init(argc,argv)<0)
            rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");
        init_memory_buf();
        init_port();

        freebsd_init();
    }

    void load_config(boost::program_options::variables_map& po){
        glb_dpdk_config.proc_type=po["proc-type"].as<std::string>();
        glb_dpdk_config.proc_id=po["proc-id"].as<std::string>();
    }

    void load_config(boost::property_tree::ptree& t){
        glb_dpdk_config.lcore_mask=t.get<std::string>("dpdk.lcore_mask");
        //glb_dpdk_config.base_virtaddr=(t.find("dpdk.base_virtaddr")==t.not_found())?"":t.get<std::string>("dpdk.base_virtaddr");
        glb_dpdk_config.nb_channel=t.get<int>("dpdk.nb_channel");
        glb_dpdk_config.promiscuous=t.get<int>("dpdk.promiscuous");
        glb_dpdk_config.numa_on=t.get<int>("dpdk.numa_on");
        glb_dpdk_config.tso=t.get<int>("dpdk.tso");
        glb_dpdk_config.tx_csum_offoad_skip=t.get<int>("dpdk.tx_csum_offoad_skip");
        glb_dpdk_config.pkt_tx_delay=t.get<unsigned>("dpdk.pkt_tx_delay");

        for(boost::property_tree::ptree::value_type& v:t.get_child("ports")){
            port_cfg cfg;
            cfg.addr=v.second.get<std::string>("addr");
            cfg.netmask=v.second.get<std::string>("netmask");
            cfg.broadcast=v.second.get<std::string>("broadcast");
            cfg.gateway=v.second.get<std::string>("gateway");
            glb_dpdk_config.port_config.push_back(cfg);
        }

        for(boost::property_tree::ptree::value_type& v:t.get_child("freebsd.boot")){
            auto k=v.first;
            auto x=v.second.data();
            glb_freebsd_config.boot_config[k]=x;
        }
    }
}
#endif //PUMP_INIT_HH
