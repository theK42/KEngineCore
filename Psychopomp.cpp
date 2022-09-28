#include "Psychopomp.h"
#include <assert.h>

KEngineCore::Psychopomp::Psychopomp()
{
}

KEngineCore::Psychopomp::~Psychopomp()
{
	Deinit();
}

void KEngineCore::Psychopomp::Init()
{
	assert(mAppointments.empty());
}

void KEngineCore::Psychopomp::Deinit()
{
	Update();
}

void KEngineCore::Psychopomp::Update()
{
	for (auto appointmentPair : mAppointments)
	{
		appointmentPair.second();
	}
	mAppointments.clear();
}

void KEngineCore::Psychopomp::ScheduleAppointment(void* entity, std::function<void()> deathDelegate)
{
	mAppointments[entity] = deathDelegate;
}
