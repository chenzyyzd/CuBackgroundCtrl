#include "CuBackgroundCtrl.h"

CuBackgroundCtrl::CuBackgroundCtrl(const std::string &configPath) : configPath_(configPath), modules_() { }

CuBackgroundCtrl::~CuBackgroundCtrl() { }

void CuBackgroundCtrl::Run()
{
	try {
		Main_();
	} catch (const std::exception &e) {
		const auto &logger = CuLogger::GetLogger();
		logger->Error("The program encounters an error during runtime.");
		logger->Error("Exception Thrown: %s", e.what());
		std::exit(0);
	}
}

void CuBackgroundCtrl::Main_()
{
	const auto &logger = CuLogger::GetLogger();
	
	modules_.emplace_back(new BackgroundController(configPath_));
	modules_.emplace_back(new ConfigWatcher(configPath_));
	modules_.emplace_back(new CgroupWatcher());
	for (const auto &module : modules_) {
		module->Start();
	}

	WriteFile("/dev/cpuset/system-background/cgroup.procs", StrMerge("%d\n", getpid()));
	WriteFile("/dev/cpuctl/cgroup.procs", StrMerge("%d\n", getpid()));
	WriteFile("/dev/stune/cgroup.procs", StrMerge("%d\n", getpid()));

	logger->Info("Daemon Running (pid=%d).", getpid());
}
