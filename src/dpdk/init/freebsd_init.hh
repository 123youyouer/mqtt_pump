//
// Created by root on 2020/3/23.
//

#ifndef PUMP_FREEBSD_INIT_HH
#define PUMP_FREEBSD_INIT_HH

#include <stdlib.h>
#include <sys/pcpu.h>
namespace engine::dpdk{
    struct freebsd_config{
        std::map<std::string,std::string> boot_config;
        std::map<std::string,std::string> sysctl_config;
    };

    freebsd_config glb_freebsd_config;
    long physmem;

    int
    freebsd_init(void)
    {
        int boot_pages;
        unsigned int num_hash_buckets;
        char tmpbuf[32] = {0};
        void *bootmem;
        int error;

        snprintf(tmpbuf, sizeof(tmpbuf), "%u", ff_global_cfg.freebsd.hz);
        error = setenv("kern.hz", tmpbuf, 1);
        if (error != 0) {
            rte_exit(EXIT_FAILURE,"kern_setenv failed: kern.hz=%s\n", tmpbuf);
        }
        for(auto i=glb_freebsd_config.boot_config.begin();i!=glb_freebsd_config.boot_config.end();++i){
            if (setenv(i->first.c_str(),i->second.c_str(),1)!=0)
                printf("kern_setenv failed: %s=%s\n",i->first.c_str(), i->second.c_str());
        }

        physmem = ff_global_cfg.freebsd.physmem;

        pcpup = malloc(std::sizeof(pcpu), M_DEVBUF, M_ZERO);
        pcpu_init(pcpup, 0, sizeof(pcpu));
        CPU_SET(0, &all_cpus);

        ff_init_thread0();

        boot_pages = 16;
        bootmem = (void *)kmem_malloc(NULL, boot_pages*PAGE_SIZE, M_ZERO);
        uma_startup(bootmem, boot_pages);
        uma_startup2();

        num_hash_buckets = 8192;
        uma_page_slab_hash = (struct uma_page_head *)kmem_malloc(NULL, sizeof(struct uma_page)*num_hash_buckets, M_ZERO);
        uma_page_mask = num_hash_buckets - 1;

        mutex_init();
        mi_startup();
        sx_init(&proctree_lock, "proctree");
        ff_fdused_range(ff_global_cfg.freebsd.fd_reserve);

        cur = ff_global_cfg.freebsd.sysctl;
        while (cur) {
            error = kernel_sysctlbyname(curthread, cur->name, NULL, NULL,
                                        cur->value, cur->vlen, NULL, 0);

            if (error != 0) {
                printf("kernel_sysctlbyname failed: %s=%s, error:%d\n",
                       cur->name, cur->str, error);
            }

            cur = cur->next;
        }

        return (0);
    }

}
#endif //PUMP_FREEBSD_INIT_HH
