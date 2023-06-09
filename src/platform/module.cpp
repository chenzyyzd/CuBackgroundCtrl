#include "platform/module.h"

Module::Module() { }

Module::~Module() { }

void Module::Broadcast_SetBroadcastReceiver(const std::string &broadcastName, const Broadcast::BroadcastReceiver &br)
{
	Broadcast::GetInstance()->SetBroadcastReceiver(broadcastName, br);
}

void Module::Broadcast_SendBroadcast(const std::string &broadcastName, const void* data)
{
	Broadcast::GetInstance()->SendBroadcast(broadcastName, data);
}
