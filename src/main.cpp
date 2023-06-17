#include "CuBackgroundCtrl.h"
#include "utils/CuLogger.h"
#include "utils/cu_misc.h"

constexpr char DAEMON_NAME[] = "CuBackgroundCtrl";
constexpr int MIN_KERNEL_VERSION = 318000;
constexpr int MIN_ANDROID_SDK = 28;

void ResetArgv(int argc, char* argv[])
{
	int argLen = 0;
	for (int i = 0; i < argc; i++) {
		argLen += strlen(argv[i]) + 1;
	}
	memset(argv[0], 0, argLen);
	strcpy(argv[0], DAEMON_NAME);
}

void KillOldDaemon()
{
	int myPid = getpid();
	DIR* dir = opendir("/proc");
	if (dir) {
		struct dirent* entry = nullptr;
		while ((entry = readdir(dir)) != nullptr) {
			if (IsPathExist(StrMerge("/proc/%s/cmdline", entry->d_name))) {
				int taskPid = atoi(entry->d_name);
				if (taskPid > 0 && taskPid < 32768) {
					std::string taskName = GetTaskName(taskPid);
					if (taskName == DAEMON_NAME && taskPid != myPid) {
						kill(taskPid, SIGINT);
					}
				}
			}
		}
		closedir(dir);
	}
}

void DaemonMain(const std::string &configPath)
{
	daemon(0, 0);

	const auto &logger = CuLogger::GetLogger();
	logger->Info("CuBackgroundCtrl V1 (%d) by chenzyadb.", GetCompileDateCode(__DATE__));

	if (GetLinuxKernelVersion() < MIN_KERNEL_VERSION) {
		logger->Warning("Your Linux kernel is out-of-date, may have compatibility issues.");
	}
	if (GetAndroidSDKVersion() < MIN_ANDROID_SDK) {
		logger->Warning("Your Android System is out-of-date, may have compatibility issues.");
	}
	if (!IsPathExist(configPath)) {
		logger->Error("Config file doesn't exist.");
		std::exit(0);
	}

	CuBackgroundCtrl daemon(configPath);
	daemon.Run();

	for (;;) {
		sleep(INT_MAX);
	}
}

int main(int argc, char* argv[])
{
	std::string option = "";
	std::string configPath = "";
	std::string logPath = "";
	if (argc == 2) {
		option = argv[1];
	} else if (argc == 4) {
		option = argv[1];
		configPath = argv[2];
		logPath = argv[3];
	}
	ResetArgv(argc, argv);
	SetThreadName(DAEMON_NAME);

	if (option == "-R" && argc == 4) {
		std::cout << "Daemon Start." << std::endl;
		CuLogger::CreateLogger(CuLogger::LOG_DEBUG, logPath);
		KillOldDaemon();
		DaemonMain(configPath);
	} else {
		std::cout << "Wrong Input." << std::endl;
	}

	return 0;
}
