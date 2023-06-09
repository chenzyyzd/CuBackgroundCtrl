#pragma once

#include <iostream>
#include <exception>
#include <stdexcept>

#include "platform/module.h"
#include "platform/singleton.h"
#include "modules/cgroup_watcher.h"
#include "modules/config_watcher.h"
#include "modules/background_controller.h"
#include "utils/cu_misc.h"
#include "utils/CuLogger.h"

class CuBackgroundCtrl
{
	public:
		CuBackgroundCtrl(const std::string &configPath);
		~CuBackgroundCtrl();
		void Run();

	private:
		std::string configPath_;
		std::vector<Module*> modules_;

		void Main_();
};
