#include "background_controller.h"

BackgroundController::BackgroundController(const std::string &configPath) : 
    Module(), 
    configPath_(configPath),
    policyMap_(),
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
    const std::regex appProcRegex("^\\w*(\\.\\w*)+\\s*$");
    const std::regex appCoProcRegex("^\\w*(\\.\\w*)+:");

    for (;;) {
        {
			std::unique_lock<std::mutex> lck(mtx_);
			while (!unblocked_) {
				cv_.wait(lck);
			}
			unblocked_ = false;
		}
        std::vector<int> tasksList{};
        {
            const auto &lines = StrSplit(ReadFile("/dev/cpuset/background/cgroup.procs"), "\n");
            for (const auto &line : lines) {
                int pid = StringToInteger(line);
                if (pid > 0 && pid < 32768) {
                    tasksList.emplace_back(pid);
                }
            }
        }
        for (const auto &pid : tasksList) {
            std::string taskName = GetTaskName(pid);
            if (std::regex_search(taskName, appCoProcRegex)) {
                taskName = GetPrevString(taskName, ':');
            }
            if (std::regex_search(taskName, appProcRegex)) {
                int taskType = GetTaskType(pid);
                int policy = 0;
                if (policyMap_.size() > 0) {
                    if (policyMap_.count(taskName) == 1) {
                        policy = policyMap_.at(taskName);
                    }
                }
                switch (taskType) {
                    case TASK_KILLABLE:
                        if (policy >= 0) {
                            kill(pid, SIGKILL);
                            logger_->Debug("App \"%s\" (pid=%d) has been killed.", taskName.c_str(), pid);
                        }
                        break;
                    case TASK_BACKGROUND:
                        if (policy >= 1) {
                            kill(pid, SIGKILL);
                            logger_->Debug("App \"%s\" (pid=%d) has been killed.", taskName.c_str(), pid);
                        } 
                        break;
                    case TASK_SERVICE:
                        if (policy == 2) {
                            kill(pid, SIGKILL);
                            logger_->Debug("App \"%s\" (pid=%d) has been killed.", taskName.c_str(), pid);
                        }
                        break;
                    default:
                        break;
                }
            }
        }
        usleep(500000);
    }
}

void BackgroundController::LoadConfig_()
{
    policyMap_.clear();
    const auto &lines = StrSplit(ReadFileEx(configPath_), "\n");
    for (const auto &line : lines) {
        if (line.size() > 4 && line[0] != '#') {
            std::string packageName{};
            int policy = 0;
            packageName.resize(line.size());
            sscanf(line.c_str(), "%s %d", &packageName[0], &policy);
            packageName.resize(strlen(packageName.c_str()));
            policyMap_[packageName] = policy;
        }
    }
    logger_->Info("Config updated.");
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