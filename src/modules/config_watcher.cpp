#include "config_watcher.h"
#include <sys/inotify.h>

ConfigWatcher::ConfigWatcher(const std::string &configPath) : Module(), configPath_(configPath), thread_() { }

ConfigWatcher::~ConfigWatcher() { }

void ConfigWatcher::Start()
{
    thread_ = std::thread(std::bind(&ConfigWatcher::Main_, this));
    thread_.detach();
}

void ConfigWatcher::Main_()
{
    SetThreadName("ConfigWatcher");
    const auto &logger = CuLogger::GetLogger();

    int fd = inotify_init();
	if (fd < 0) {
		logger->Error("Failed to init inotify.");
		std::exit(0);
	}
    if (inotify_add_watch(fd, configPath_.c_str(), IN_MODIFY) < 0) {
        logger->Error("Failed to add watch.");
		std::exit(0);
    }

    for (;;) {
        struct inotify_event watchEvent{};
		read(fd, &watchEvent, sizeof(struct inotify_event));
		if (watchEvent.mask == IN_MODIFY) {
            Broadcast_SendBroadcast("ConfigWatcher.ConfigModified", nullptr);
        }
    }

    close(fd);
}
