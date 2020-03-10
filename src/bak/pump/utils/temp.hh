//
// Created by null on 20-1-29.
//

#ifndef PROJECT_TEMP_HH
#define PROJECT_TEMP_HH

#include <boost/noncopyable.hpp>
#include <cstdio>
#include <iostream>

namespace temp{
    struct bbbb:boost::noncopyable{
        char* szbuf;
    public:
        bbbb():szbuf(new char[128]){
            sprintf(szbuf,"000");
        }
    };

    struct nocpy:boost::noncopyable{
        char* szbuf;
        bbbb b;
    public:
        void out(){
            std::cout<<szbuf<<std::endl;
        }
        nocpy(const char* sz):szbuf(new char[128]){
            sprintf(szbuf,sz);
        }
        nocpy(const nocpy&& n):szbuf(n.szbuf){
            // n.szbuf= nullptr;
        };
        nocpy(nocpy&& n):szbuf(n.szbuf){
            n.szbuf= nullptr;
        };
        nocpy():szbuf(new char[128]){}
        virtual ~nocpy(){
            delete(szbuf);
        }
    };
}
#endif //PROJECT_TEMP_HH
