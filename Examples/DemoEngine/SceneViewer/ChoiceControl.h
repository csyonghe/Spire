#ifndef CHOICE_CONTROL_H
#define CHOICE_CONTROL_H

#include "CoreLib/WinForm.h"
#include "CoreLib/Graphics.h"
#include "CoreLib/LibGL.h"
#include "Spire.h"

namespace SceneViewer
{
	using namespace CoreLib::WinForm;
	using namespace VectorMath;

	class IChoiceControl
	{
	public:
		virtual List<String> GetShaders() = 0;
		virtual List<Spire::Compiler::ShaderChoice> GetChoices(String shader, const EnumerableDictionary<String, Spire::Compiler::ShaderChoiceValue> & existingChoices) = 0;
		virtual void RecompileShader(String shader, const String & schedule) = 0;
		virtual GL::Texture2D RenderFrame() = 0;
		virtual EnumerableDictionary<String, GL::Texture2D> GetPrecomputedTextures(String shader) = 0;
	};

	class ChoiceForm : public GraphicsUI::Form
	{
	private:
		WinForm::GLForm * ownerForm = nullptr;
		List<Vec4> referenceFrame;
		List<Vec4> ReadFrameData(GL::Texture2D tex);
		float currentBestValue = 1e30f;
		EnumerableDictionary<String, Spire::Compiler::ShaderChoiceValue> currentBestChoices;
		float MeasureError(List<Vec4> & frame0, List<Vec4> & frame1);
	private:
		bool disableChoiceChangeCapture = false;
		IChoiceControl * choiceControl;
		GraphicsUI::Button *applyButton, *resetButton, *autoTuneButton, *autoTuneTexButton, *saveScheduleButton;
		GraphicsUI::TextBox *timeBudgetTextBox;
		GraphicsUI::VScrollPanel *scrollPanel;
		GraphicsUI::ListBox *shaderBox;
		GraphicsUI::CheckBox *autoRecompileCheckBox;
		EnumerableDictionary<GraphicsUI::ComboBox*, String> comboBoxChoiceNames;
		EnumerableDictionary<String, GraphicsUI::ComboBox*> choiceComboBoxes;
		EnumerableDictionary<String, GraphicsUI::CheckBox*> choiceCheckBoxes;
		StringBuilder autotuningLog;
		String currentShaderName;
		EnumerableDictionary<String, Spire::Compiler::ShaderChoiceValue> existingChoices;
		EnumerableDictionary<String, EnumerableDictionary<String, String>> additionalAttribs;
		void InitUI();
		void UpdateChoicePanel(String shaderName);
		void UpdateChoiceOptions(GraphicsUI::ComboBox * cmb);
		void Recompile();
		int AutotuneHelper(HashSet<String> & selectedChoices, EnumerableDictionary<String, Spire::Compiler::ShaderChoiceValue>& currentChoices, float timeBudget, bool countOnly = false);
		void Autotune(float timeBudget);
		void AutotuneTex();
		void ResetButton_Clicked(GraphicsUI::UI_Base *);
		void AutotuneButton_Clicked(GraphicsUI::UI_Base *);
		void AutotuneTexButton_Clicked(GraphicsUI::UI_Base *);
		void SaveScheduleButton_Clicked(GraphicsUI::UI_Base *);
		void ApplyButton_Clicked(GraphicsUI::UI_Base *);
		void ChoiceForm_OnResize(GraphicsUI::UI_Base *);
		void SelectedShaderChanged(GraphicsUI::UI_Base *);
		void ChoiceComboBox_Changed(GraphicsUI::UI_Base *);
		void PipelineBox_Changed(GraphicsUI::UI_Base *);
	public:
		Event<String> ShaderChanged;
		ChoiceForm(IChoiceControl * pChoiceControl, WinForm::GLForm * ownerForm, GraphicsUI::UIEntry * entry);
		void Update();

	};
}

#endif