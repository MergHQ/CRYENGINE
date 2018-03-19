// Copyright 2001-2016 Crytek GmbH. All rights reserved.

#pragma once

#include <CrySystem/File/IFileChangeMonitor.h>
#include <CryCore/ToolsHelpers/ResourceCompilerHelper.h>

namespace AssetManagerHelpers
{

bool IsFileOpened(const string& path);

struct RCLogger : IResourceCompilerListener
{
	virtual void OnRCMessage(MessageSeverity severity, const char* szText) override;
};

// A FIFO data processing queue with the ability to wait for deferred processing of items without blocking the calling thread.
class CProcessingQueue
{
public:
	typedef std::function<bool(const string& /*key*/)> Predicate;

public:
	CProcessingQueue() = default;

	// Inserts a new item into the processing queue.
	// The predicate object must return true if the item should be removed from the queue,
	// if the function object returns false, the queue will try to check the item again, somewhat later.
	// In any case the items will be processed in order of adding to the queue (first-in, first-out).
	void ProcessItemAsync(const string& key, Predicate fn);

	// Inserts a new item into the processing queue, if the queue doesn't already contain an item with an equivalent key.
	// see ProcessFileAsync
	void ProcessItemUniqueAsync(const string& key, Predicate fn);

private:
	void ProcessQueue();

private:
	std::deque<std::pair<string, Predicate>> m_queue;
};

};
