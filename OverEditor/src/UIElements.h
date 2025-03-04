#pragma once

#include <OverEngine/Renderer/Texture.h>
#include <OverEngine/Scene/Entity.h>

#include <OverEngine/ImGui/ExtraImGui.h>
#include <imgui.h>

namespace OverEditor
{
	using namespace OverEngine;

	/**
	 * A class which contains OverEditor's common
	 * Graphical User Interface elements.
	 */

	class UIElements
	{
	public:
		template <typename T>
		bool static BeginComponentEditor(Entity entity, const char* headerName, uint32_t componentTypeID);

		void static BeginFieldGroup() { ImGui::Columns(2); }
		void static EndFieldGroup() { ImGui::Columns(1); }

		static bool CheckboxField(const char* fieldName, const char* fieldID, bool* value);

		template <typename FlagType>
		static bool CheckboxFlagsField(const char* fieldName, const char* fieldID, FlagType* flags, FlagType value);

		// Useful for Vectors .etc
		static bool DragFloatNField(const char* fieldName, const char* fieldID, float* value, uint32_t count = 1, float speed = 1.0f, float min = 0.0f, float max = 0.0f, const char* format = "%.3f");
		static bool DragFloatField(const char* fieldName, const char* fieldID, float* value, float speed = 1.0f, float min = 0.0f, float max = 0.0f, const char* format = "%.3f");
		static bool DragFloat2Field(const char* fieldName, const char* fieldID, float value[2], float speed = 1.0f, float min = 0.0f, float max = 0.0f, const char* format = "%.3f");
		static bool DragFloat3Field(const char* fieldName, const char* fieldID, float value[3], float speed = 1.0f, float min = 0.0f, float max = 0.0f, const char* format = "%.3f");
		static bool DragFloat4Field(const char* fieldName, const char* fieldID, float value[4], float speed = 1.0f, float min = 0.0f, float max = 0.0f, const char* format = "%.3f");

		// For RGBA colors
		static bool Color4Field(const char* fieldName, const char* fieldID, float value[4]);

		// Combobox useful for enums
		using EnumValues = Map<int, String>;

		template <typename T>
		static bool BasicEnum(const char* fieldName, const char* fieldID, EnumValues& values, T* currentValue, const ImGuiSelectableFlags& flags = 0);

		// Drag and drop
		static void Texture2DField(const char* fieldName, const char* fieldID, Ref<Texture2D>& texture);
		static void Texture2DDragSource(const Ref<Texture2D>& texture, const char* name, bool preview = false);

		// Tooltip
		template <typename F>
		static void Tooltip(F func);
	};

	///////////////////////////////////////////////
	// Implementation /////////////////////////////
	///////////////////////////////////////////////

	template <typename T>
	bool UIElements::BeginComponentEditor(Entity entity, const char* headerName, uint32_t componentTypeID)
	{
		char txt[64];
		constexpr auto txtSize = OE_ARRAY_SIZE(txt);

		sprintf_s(txt, txtSize, "%s##%i", "X", componentTypeID);

		bool componentRemoved = false;
		if (ImGui::Button(txt))
		{
			entity.RemoveComponent<T>();
			componentRemoved = true;
		}

		sprintf_s(txt, txtSize, "##%i", componentTypeID);
		if (!componentRemoved)
		{
			ImGui::SameLine();
			ImGui::Checkbox(txt, &entity.GetComponent<T>().Enabled);
		}

		ImGui::SameLine();

		sprintf_s(txt, txtSize, "##MOVE_COMPONENT_UP%i", componentTypeID);

		if (ImGui::ArrowButton(txt, ImGuiDir_Up))
		{
			auto& componentList = entity.GetComponentsTypeIDList();
			auto it = STD_CONTAINER_FIND(componentList, componentTypeID);
			if (it != componentList.begin())
			{
				componentList[it - componentList.begin()] = componentList[it - componentList.begin() - 1];
				componentList[it - componentList.begin() - 1] = componentTypeID;
			}
		}

		ImGui::SameLine();

		sprintf_s(txt, txtSize, "##MOVE_COMPONENT_DOWN%i", componentTypeID);

		if (ImGui::ArrowButton(txt, ImGuiDir_Down))
		{
			auto& componentList = entity.GetComponentsTypeIDList();
			auto it = STD_CONTAINER_FIND(componentList, componentTypeID);
			if (it != componentList.end() - 1)
			{
				componentList[it - componentList.begin()] = componentList[it - componentList.begin() + 1];
				componentList[it - componentList.begin() + 1] = componentTypeID;
			}
		}

		ImGui::SameLine();
		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		return !componentRemoved && ImGui::CollapsingHeader(headerName);
	}

	template <typename FlagType>
	bool UIElements::CheckboxFlagsField(const char* fieldName, const char* fieldID, FlagType* flags, FlagType value)
	{
		ImGui::TextUnformatted(fieldName);
		ImGui::NextColumn();

		ImGui::PushItemWidth(-1);

		bool v = ((*flags & value) == value);
		bool pressed = ImGui::Checkbox(fieldID, &v);
		if (pressed)
		{
			if (v)
				*flags |= value;
			else
				*flags &= ~value;
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return pressed;
	}

	template <typename T>
	bool UIElements::BasicEnum(const char* fieldName, const char* fieldID, EnumValues& values, T* currentValue, const ImGuiSelectableFlags& flags)
	{
		ImGui::TextUnformatted(fieldName);
		ImGui::NextColumn();

		ImGui::PushItemWidth(-1);
		bool changed = false;
		if (ImGui::BeginCombo(fieldID, values[(int)(*currentValue)].c_str()))
		{
			for (const auto& value : values)
			{
				const bool selected = (value.first == *currentValue);
				if (ImGui::Selectable(value.second.c_str(), selected, flags))
				{
					*currentValue = value.first;
					changed = true;
				}

				// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
				if (selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		ImGui::PopItemWidth();

		ImGui::NextColumn();

		return changed;
	}

	template <typename F>
	void UIElements::Tooltip(F func)
	{
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			func();
			ImGui::EndTooltip();
		}
	}
}
