#pragma once

#include "platform/broadcast.h"

class Module
{
	public:
		Module();
		virtual ~Module();
		virtual void Start() = 0;

	protected:
		void Broadcast_SetBroadcastReceiver(const std::string &broadcastName, const Broadcast::BroadcastReceiver &br);
		void Broadcast_SendBroadcast(const std::string &broadcastName, const void* data);
};
