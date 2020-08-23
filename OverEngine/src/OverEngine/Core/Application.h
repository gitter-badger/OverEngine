#pragma once

#include "OverEngine/Core/Core.h"
#include "OverEngine/Core/Window.h"
#include "OverEngine/Core/Time/Time.h"

#include "OverEngine/Layers/LayerStack.h"
#include "OverEngine/Events/Event.h"
#include "OverEngine/Events/ApplicationEvent.h"

namespace OverEngine
{
	class ImGuiLayer;

	class Application
	{
	public:
		Application(String name = "OverEngine", bool InitRenderer = true);
		virtual ~Application();

		void Run();
		inline void Close() { m_Running = false; }

		void OnEvent(Event& e);

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);

		inline static Application& Get() { return *s_Instance; }
		ImGuiLayer* GetImGuiLayer() { return m_ImGuiLayer; }
		inline Window& GetMainWindow() { return *m_Window; }
	private:
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);

		Scope<Window> m_Window;

		bool m_Running   = true;
		bool m_Minimized = false;
		bool m_UseInternalRenderer = false;

		LayerStack  m_LayerStack;
		ImGuiLayer* m_ImGuiLayer;
	protected:
		bool m_ImGuiEnabled = false;
	private:
		static Application* s_Instance;
	};

	// To be defined in CLIENT
	Application* CreateApplication(int argc, char** argv);
}