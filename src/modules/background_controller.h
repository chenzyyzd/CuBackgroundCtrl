#pragma once

#include <unordered_map>
#include <thread>
#include <regex>
#include "platform/module.h"
#include "utils/cu_misc.h"
#include "utils/CuLogger.h"

class BackgroundController : public Module 
{
    public:
        BackgroundController(const std::string &configPath);
        ~BackgroundController();
        void Start();

    private:
        std::regex pkgNameRegex_;
        std::string configPath_;
        std::vector<std::string> whiteList_;
        CuLogger* logger_;
        std::thread thread_;
        std::condition_variable cv_;
        std::mutex mtx_;
        bool unblocked_;

        void ControllerMain_();
        void LoadConfig_();
        void ConfigModified_(const void* data);
        void CgroupModified_(const void* data);
};
