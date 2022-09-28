#pragma once
#include<functional>
#include<map>

namespace KEngineCore {
	class Psychopomp
	{
	public:
		Psychopomp();
		~Psychopomp();
		void Init();
		void Deinit();

		void Update();

		void ScheduleAppointment(void* entity, std::function<void()> deathDelegate);
	private:
		std::map<void*, std::function<void()>> mAppointments;
	};
}
