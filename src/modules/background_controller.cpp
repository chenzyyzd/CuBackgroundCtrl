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

    const std::regex appProcRegex("^[\\w]+([.][\\w]+)+");
    std::unordered_map<std::string, int> stateTraceMap{};

    for (;;) {
        {
			std::unique_lock<std::mutex> lck(mtx_);
			while (!unblocked_) {
				cv_.wait(lck);
			}
			unblocked_ = false;
		}

        std::unordered_map<int, std::string> backgroundTasks{};
        std::unordered_map<int, std::string> foregroundTasks{};
        {
            const auto &lines = StrSplit(ReadFile("/dev/cpuset/background/cgroup.procs"), "\n");
            for (const auto &line : lines) {
                int pid = StringToInteger(line);
                if (pid > 0 && pid < 32768) {
                    backgroundTasks[pid] = GetTaskName(pid);
                }
            }
        }
        {
            const auto &lines = StrSplit(ReadFile("/dev/cpuset/foreground/cgroup.procs"), "\n");
            for (const auto &line : lines) {
                int pid = StringToInteger(line);
                if (pid > 0 && pid < 32768) {
                    foregroundTasks[pid] = GetTaskName(pid);
                }
            }
        }
        {
            const auto &lines = StrSplit(ReadFile("/dev/cpuset/top-app/cgroup.procs"), "\n");
            for (const auto &line : lines) {
                int pid = StringToInteger(line);
                if (pid > 0 && pid < 32768) {
                    foregroundTasks[pid] = GetTaskName(pid);
                }
            }
        }

        auto prevStateTraceMap = stateTraceMap;
        {
            for (auto &[pkgName, state] : stateTraceMap) {
                if (state != STATE_KILLED) {
                    state = STATE_BACKGROUND;
                }
            }
            for (const auto &[pid, taskName] : backgroundTasks) {
                if (std::regex_search(taskName, appProcRegex)) {
                    stateTraceMap[taskName] = STATE_BACKGROUND;
                }
            }
            for (const auto &[pid, taskName] : foregroundTasks) {
                if (std::regex_search(taskName, appProcRegex)) {
                    stateTraceMap[taskName] = STATE_FOREGROUND;
                }
            }
        }

        for (const auto &[pid, taskName] : backgroundTasks) {
            if (stateTraceMap.count(taskName) != 0) {
                int policy = 0;
                if (policyMap_.size() > 0) {
                    std::string pkgName = taskName;
                    if (StrContains(pkgName, ":")) {
                        pkgName = GetPrevString(pkgName, ':');
                    }
                    if (policyMap_.count(pkgName) == 1) {
                        policy = policyMap_.at(pkgName);
                    }
                }
                int prevState = STATE_BACKGROUND;
                if (prevStateTraceMap.count(taskName) == 1) {
                    prevState = prevStateTraceMap.at(taskName);
                }
                if (prevState == STATE_KILLED) {
                    if (policy != POLICY_WHITELIST) {
                        kill(pid, SIGINT);
                        stateTraceMap[taskName] = STATE_KILLED;
                    }
                } else {
                    int taskType = GetTaskType(pid);
                    if (taskType == TASK_KILLABLE) {
                        if (policy != POLICY_WHITELIST) {
                            kill(pid, SIGINT);
                            stateTraceMap[taskName] = STATE_KILLED;
                        }
                    } else if (taskType == TASK_BACKGROUND) {
                        if (policy == POLICY_NORMAL || policy == POLICY_STRICT) {
                            kill(pid, SIGINT);
                            stateTraceMap[taskName] = STATE_KILLED;
                        }
                    } else if (taskType == TASK_SERVICE) {
                        if (policy == POLICY_STRICT) {
                            kill(pid, SIGINT);
                            stateTraceMap[taskName] = STATE_KILLED;
                        }
                    }
                }
            }
        }
        sleep(1);
    }
}

void BackgroundController::LoadConfig_()
{
    const std::regex policyRegex("^[\\w]+([.][\\w]+)+[\\s]+(-1|0|1|2)[\\s]*$");
    policyMap_.clear();
    const auto &lines = StrSplit(ReadFileEx(configPath_), "\n");
    for (const auto &line : lines) {
        if (std::regex_search(line, policyRegex)) {
            std::string packageName{};
            int policy = 0;
            packageName.resize(line.size());
            sscanf(line.c_str(), "%s %d", &packageName[0], &policy);
            packageName.resize(strlen(packageName.c_str()));
            policyMap_[packageName] = policy;
        }
    }
    logger_->Info("Config updated.");
    for (const auto &[pkgName, policy] : policyMap_) {
        switch (policy) {
            case POLICY_WHITELIST:
                logger_->Info("App \"%s\": policy whitelist.", pkgName.c_str());
                break;
            case POLICY_DEFAULT:
                logger_->Info("App \"%s\": policy default.", pkgName.c_str());
                break;
            case POLICY_NORMAL:
                logger_->Info("App \"%s\": policy normal.", pkgName.c_str());
                break;
            case POLICY_STRICT:
                logger_->Info("App \"%s\": policy strict.", pkgName.c_str());
                break;
            default:
                break;
        }
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
