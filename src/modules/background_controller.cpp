#include "background_controller.h"

constexpr int STATE_KILLED = 0;
constexpr int STATE_BACKGROUND = 1;
constexpr int STATE_FOREGROUND = 2;

constexpr int POLICY_WHITELIST = -1;
constexpr int POLICY_DEFAULT = 0;
constexpr int POLICY_NORMAL = 1;
constexpr int POLICY_STRICT = 2;

BackgroundController::BackgroundController(const std::string &configPath) : 
    Module(), 
    pkgNameRegex_("^[\\w]+([.][\\w]+)+[\\s]*$"),
    configPath_(configPath),
    whiteList_(),
    logger_(CuLogger::GetLogger()),
    thread_(),
    cv_(),
    mtx_(),
    unblocked_(false) { }

BackgroundController::~BackgroundController() { }

void BackgroundController::Start() 
{
    LoadConfig_();
    {
        unblocked_ = false;
        thread_ = std::thread(std::bind(&BackgroundController::ControllerMain_, this));
        thread_.detach();
    }
    {
        using namespace std::placeholders;
        Broadcast_SetBroadcastReceiver("CgroupWatcher.TopAppCgroupModified", std::bind(&BackgroundController::CgroupModified_, this, _1));
        Broadcast_SetBroadcastReceiver("CgroupWatcher.ForegroundCgroupModified", std::bind(&BackgroundController::CgroupModified_, this, _1));
        Broadcast_SetBroadcastReceiver("CgroupWatcher.BackgroundCgroupModified", std::bind(&BackgroundController::CgroupModified_, this, _1));
        Broadcast_SetBroadcastReceiver("ConfigWatcher.ConfigModified", std::bind(&BackgroundController::ConfigModified_, this, _1));
    }
}

void BackgroundController::ControllerMain_()
{
    SetThreadName("ControllerMain");

    std::vector<int> freezedTasks{};
    for (;;) {
        {
            std::unique_lock<std::mutex> lck(mtx_);
            while (!unblocked_) {
                cv_.wait(lck);
            }
            unblocked_ = false;
        }
        {
            std::unordered_map<int, std::string> backgroundTasks{};
            {
                const auto &lines = StrSplit(ReadFile("/dev/cpuset/background/cgroup.procs"), "\n");
                for (const auto &line : lines) {
                    int pid = StringToInteger(line);
                    if (pid > 0 && pid < 32768) {
                        backgroundTasks[pid] = GetTaskName(pid);
                    }
                }
            }
        
            std::vector<std::string> needKillApps{};
            std::vector<std::string> needFreezeApps{};
            for (const auto &[pid, taskName] : backgroundTasks) {
                if (std::regex_search(taskName, pkgNameRegex_)) {
                    const auto &iter = std::find(whiteList_.begin(), whiteList_.end(), taskName);
                    if (iter == whiteList_.end()) {
                        int taskType = GetTaskType(pid);
                        if (taskType == TASK_KILLABLE) {
                            needKillApps.emplace_back(taskName);
                        } else if (taskType == TASK_BACKGROUND) {
                            needFreezeApps.emplace_back(taskName);
                        }
                    }
                }
            }

            for (const auto &pkgName : needKillApps) {
                for (const auto &[pid, taskName] : backgroundTasks) {
                    if (StrContains(taskName, pkgName)) {
                        if (kill(pid, SIGTERM) == -1) {
                            kill(pid, SIGKILL);
                        }
                        {
                            const auto &iter = std::find(freezedTasks.begin(), freezedTasks.end(), pid);
                            if (iter != freezedTasks.end()) {
                                freezedTasks.erase(iter);
                            }
                        }
                    }
                }
            }
            {
                std::vector<int> freezingTasks{};
                for (const auto &pkgName : needFreezeApps) {
                    for (const auto &[pid, taskName] : backgroundTasks) {
                        if (StrContains(taskName, pkgName)) {
                            freezingTasks.emplace_back(pid);
                        }
                    }
                }
                auto prevFreezedTasks = freezedTasks;
                freezedTasks.clear();
                for (const int &pid : freezingTasks) {
                    kill(pid, SIGSTOP);
                    freezedTasks.emplace_back(pid);
                }
                for (const int &pid : prevFreezedTasks) {
                    if (std::find(freezedTasks.begin(), freezedTasks.end(), pid) == freezedTasks.end()) {
                        kill(pid, SIGCONT);
                    }
                }
            }
        }

        usleep(500000);
    }
}

void BackgroundController::LoadConfig_()
{
    whiteList_.clear();
    const auto &lines = StrSplit(ReadFileEx(configPath_), "\n");
    for (const auto &line : lines) {
        if (std::regex_search(line, pkgNameRegex_)) {
            whiteList_.emplace_back(TrimStr(line));
        }
    }
    logger_->Info("Config updated.");
    for (const auto &item : whiteList_) {
        logger_->Info("WhiteList: \"%s\".", item.c_str());
    }
}

void BackgroundController::ConfigModified_(const void* data)
{
    LoadConfig_();
}

void BackgroundController::CgroupModified_(const void* data)
{
    std::unique_lock<std::mutex> lck(mtx_);
    unblocked_ = true;
    cv_.notify_all();
}
