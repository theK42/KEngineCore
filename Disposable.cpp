#include "Disposable.h"
#include <assert.h>

KEngineCore::DisposableGroup::DisposableGroup()
{
}

KEngineCore::DisposableGroup::~DisposableGroup()
{
	Deinit();
}

void KEngineCore::DisposableGroup::Init()
{
	assert(mMembers.empty());
}

void KEngineCore::DisposableGroup::Deinit()
{
	for (auto* disposable : mMembers)
	{
		disposable->Deinit();
	}
	mMembers.clear();
}

void KEngineCore::DisposableGroup::AddDisposable(Disposable* member)
{
	mMembers.insert(member);
}
