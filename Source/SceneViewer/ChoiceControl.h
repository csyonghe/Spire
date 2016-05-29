#pragma once

#include "CoreLib/WinForm/WinForm.h"
#include "CoreLib/WinForm/WinButtons.h"
#include "CoreLib/WinForm/WinApp.h"
#include "CoreLib/WinForm/WinCommonDlg.h"
#include "CoreLib/WinForm/WinTextBox.h"
#include "CoreLib/WinForm/WinListBox.h"
#include "LibGL/OpenGLHardwareRenderer.h"
#include "ShaderCompiler.h"
#include "Schedule.h"

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

	class ChoiceForm : public Form
	{
	private:
		HashSet<String> availableChoices[4];
		List<Vec4> referenceFrame;
		List<Vec4> ReadFrameData(GL::Texture2D tex);
		float currentBestValue = 1e30f;
		EnumerableDictionary<String, Spire::Compiler::ShaderChoiceValue> currentBestChoices;
		float MeasureError(List<Vec4> & frame0, List<Vec4> & frame1);
	private:
		bool disableChoiceChangeCapture = false;
		IChoiceControl * choiceControl;
		RefPtr<Button> applyButton, resetButton, autoTuneButton, autoTuneTexButton, saveScheduleButton;
		RefPtr<TextBox> timeBudgetTextBox;
		RefPtr<ScrollPanel> scrollPanel;
		RefPtr<ListBox> shaderBox;
		RefPtr<ComboBox> pipelineBox;
		RefPtr<CheckBox> autoRecompileCheckBox;
		EnumerableDictionary<ComboBox*, String> comboBoxChoiceNames;
		EnumerableDictionary<String, ComboBox*> choiceComboBoxes;
		EnumerableDictionary<String, CheckBox*> choiceCheckBoxes;
		StringBuilder autotuningLog;
		String currentShaderName;
		EnumerableDictionary<String, Spire::Compiler::ShaderChoiceValue> existingChoices;
		EnumerableDictionary<String, EnumerableDictionary<String, String>> additionalAttribs;
		void InitUI();
		void UpdateChoicePanel(String shaderName);
		void UpdateChoiceOptions(ComboBox * cmb);
		void Recompile();
		int AutotuneHelper(HashSet<String> & selectedChoices, EnumerableDictionary<String, Spire::Compiler::ShaderChoiceValue>& currentChoices, float timeBudget, int pipeline, bool countOnly = false);
		void Autotune(float timeBudget);
		void AutotuneTex();
		void ResetButton_Clicked(Object * obj, EventArgs e);
		void AutotuneButton_Clicked(Object * obj, EventArgs e);
		void AutotuneTexButton_Clicked(Object * obj, EventArgs e);
		void SaveScheduleButton_Clicked(Object * obj, EventArgs e);
		void ApplyButton_Clicked(Object * obj, EventArgs e);
		void ChoiceForm_OnResize(Object * obj, EventArgs e);
		void SelectedShaderChanged(Object * obj, EventArgs e);
		void ChoiceComboBox_Changed(Object * obj, EventArgs e);
		void PipelineBox_Changed(Object *, EventArgs);
	public:
		Event<String> ShaderChanged;
		ChoiceForm(IChoiceControl * pChoiceControl);
		void Update();

	};
}