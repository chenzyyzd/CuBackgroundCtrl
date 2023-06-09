#pragma once

#include <thread>
#include "platform/module.h"
#include "utils/cu_misc.h"
#include "utils/CuLogger.h"

class ConfigWatcher : public Module 
{
    public:
        ConfigWatcher(const std::string &configPath);
        ~ConfigWatcher();
        void Start();

    private:
        std::string configPath_;
        std::thread thread_;

        void Main_();
};