//
// Created by anyone on 20-2-15.
//

#ifndef PROJECT_ENGINE_HH
#define PROJECT_ENGINE_HH

#include <string>
#include <boost/program_options.hpp>

namespace engine{
    namespace bpo=boost::program_options;

    struct engine_config{
        std::string process_type;
        std::string config_file_path;
    };

    engine_config cur_config;

    void engine_init(int argc, char* argv[]){
        bpo::options_description opt;
        bpo::variables_map vm;
        opt.add_options()
                ("ptype",bpo::value<string>(&cur_config.process_type,"process type"))
                ("config",bpo::value<string>(&cur_config.config_file_path),"config file path");

        bpo::store(bpo::parse_command_line(argc,argv,opt),vm);
        bpo::notify(vm);

    }
}
#endif //PROJECT_ENGINE_HH
