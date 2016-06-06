#include "ChoiceControl.h"
#include "CoreLib/PerformanceCounter.h"
#include "CoreLib/LibIO.h"
#include "../RealtimeEngine/Pipeline.h"

using namespace CoreLib::IO;
using namespace CoreLib::Diagnostics;
using namespace RealtimeEngine;

#define ENABLE_AUTO_TUNE
namespace SceneViewer
{
	ChoiceForm::ChoiceForm(IChoiceControl * pChoiceControl, WinForm::GLForm * pOwnerForm, GraphicsUI::UIEntry * entry)
		: GraphicsUI::Form(entry)
	{
		this->ownerForm = pOwnerForm;
		this->choiceControl = pChoiceControl;
		SetText("Choice Control");
		InitUI();
		availableChoices[0].Add(L"vs");
		availableChoices[0].Add(L"fs");
		availableChoices[1].Add(L"precomputeUniform");
		availableChoices[1].Add(L"precomputeTex");
		availableChoices[1].Add(L"precomputeVert");
		availableChoices[1].Add(L"vs");
		availableChoices[1].Add(L"fs");
		availableChoices[2].Add(L"vs");
		availableChoices[2].Add(L"lowRes");
		availableChoices[2].Add(L"fs");
		availableChoices[3].Add(L"vs");
		availableChoices[3].Add(L"objSurface");
		availableChoices[3].Add(L"lowRes");
		availableChoices[3].Add(L"fs");

		OnResize.Bind(this, &ChoiceForm::ChoiceForm_OnResize);
	}
	List<Vec4> ChoiceForm::ReadFrameData(GL::Texture2D tex)
	{
		List<Vec4> rs;
		int w, h;
		tex.GetSize(w, h);
		rs.SetSize(w*h);
		tex.GetData(0, DataType::Float4, rs.Buffer(), rs.Count()*sizeof(Vec4));
		return rs;
	}
	float ChoiceForm::MeasureError(List<Vec4>& frame0, List<Vec4>& frame1)
	{
		float error = 0.0f;
		int count = 0;
		for (int i = 0; i < frame0.Count(); i++)
		{
			if (frame0[i].w != 0.0f)
			{
				count++;
				error += fabs(frame0[i].x - frame1[i].x);
				error += fabs(frame0[i].y - frame1[i].y);
				error += fabs(frame0[i].z - frame1[i].z);
			}
		}
		if (count == 0)
			return 0.0f;
		return error / count;
	}
	void ChoiceForm::InitUI()
	{
		pipelineBox = new GraphicsUI::ComboBox(this);
		pipelineBox->AddTextItem(L"Standard");
		pipelineBox->AddTextItem(L"Precompute");
		pipelineBox->AddTextItem(L"MultiRate");
		pipelineBox->AddTextItem(L"MultiRateObjSpace");
		pipelineBox->SetSelectedIndex(0);
		pipelineBox->Posit(10, 10, 120, 25);
		pipelineBox->OnChanged.Bind(this, &ChoiceForm::PipelineBox_Changed);
		shaderBox = new GraphicsUI::ListBox(this);
		shaderBox->Posit(10, 50, 120, GetClientHeight() - 60);
		shaderBox->OnChanged.Bind(this, &ChoiceForm::SelectedShaderChanged);
		applyButton = new GraphicsUI::Button(this);
		applyButton->Posit(140, 10, 100, 30);
		applyButton->SetText(L"Recompile");
		applyButton->OnClick.Bind(this, &ChoiceForm::ApplyButton_Clicked);
		resetButton = new GraphicsUI::Button(this);
		resetButton->Posit(250, 10, 100, 30);
		resetButton->SetText(L"Reset");
		resetButton->OnClick.Bind(this, &ChoiceForm::ResetButton_Clicked);
		autoRecompileCheckBox = new GraphicsUI::CheckBox(this);
		autoRecompileCheckBox->SetText(L"Auto Recompile");
		autoRecompileCheckBox->Posit(360, 14, 150, 25);
		autoRecompileCheckBox->Checked = true;
#ifdef ENABLE_AUTO_TUNE
		timeBudgetTextBox = new GraphicsUI::TextBox(this);
		timeBudgetTextBox->SetText(L"10");
		timeBudgetTextBox->Posit(140, 52, 80, 25);
		autoTuneButton = new GraphicsUI::Button(this);
		autoTuneButton->Posit(230, 50, 100, 30);
		autoTuneButton->SetText(L"Autotune");
		autoTuneButton->OnClick.Bind(this, &ChoiceForm::AutotuneButton_Clicked);
		autoTuneTexButton = new GraphicsUI::Button(this);
		autoTuneTexButton->Posit(340, 50, 140, 30);
		autoTuneTexButton->SetText(L"Tune Texture");
		autoTuneTexButton->OnClick.Bind(this, &ChoiceForm::AutotuneTexButton_Clicked);
		saveScheduleButton = new GraphicsUI::Button(this);
		saveScheduleButton->Posit(490, 50, 100, 30);
		saveScheduleButton->SetText(L"&Save");
		saveScheduleButton->OnClick.Bind(this, &ChoiceForm::SaveScheduleButton_Clicked);
#endif
		scrollPanel = new GraphicsUI::VScrollPanel(this);
		scrollPanel->Posit(140, 90, GetClientWidth() - 120, GetClientHeight() - 100);
	}
	void ChoiceForm::UpdateChoicePanel(String shaderName)
	{
		if (shaderName.Length())
			SetText(L"Choice Control - " + shaderName);
		currentShaderName = shaderName;
		scrollPanel->ClearChildren();
		int scWidth = scrollPanel->GetClientWidth();

		existingChoices.Clear();
		additionalAttribs.Clear();
		auto choices = choiceControl->GetChoices(shaderName, existingChoices);
		int line = 0;
		comboBoxChoiceNames.Clear();
		choiceComboBoxes.Clear();
		choiceCheckBoxes.Clear();
		existingChoices.Clear();
		for (auto & choice : choices)
		{
			if (choice.Options.Count() == 1)
				continue;
			int cmbLeft = scWidth - 140;
			auto lbl = new GraphicsUI::CheckBox(scrollPanel);
			lbl->SetText(choice.ChoiceName);
			lbl->Posit(0, line * 30, cmbLeft - 5, 25);
			choiceCheckBoxes.Add(choice.ChoiceName, lbl);
			auto cmb = new GraphicsUI::ComboBox(scrollPanel);
			cmb->AddTextItem(L"(auto) " + choice.DefaultValue);
			for (auto & opt : choice.Options)
			{
				if (availableChoices[Math::Clamp(pipelineBox->SelectedIndex,0,pipelineBox->Items.Count()-1)].Contains(opt.WorldName))
					cmb->AddTextItem(opt.ToString());
			}
			comboBoxChoiceNames[cmb] = choice.ChoiceName;
			choiceComboBoxes[choice.ChoiceName] = cmb;
			cmb->SetSelectedIndex(0);
			cmb->OnChanged.Bind(this, &ChoiceForm::ChoiceComboBox_Changed);
			cmb->Posit(cmbLeft, line * 30, 130, 25);
			line++;
		}
		ChoiceForm_OnResize(this);
	}
	int GetSelIdx(GraphicsUI::ListBox * cmb)
	{
		if (cmb->SelectedIndex == -1)
			return 0;
		return cmb->SelectedIndex;
	}
	void ChoiceForm::UpdateChoiceOptions(GraphicsUI::ComboBox * cmb)
	{
		for (auto & choiceCtrl : comboBoxChoiceNames)
		{
			String option = choiceCtrl.Key->GetTextItem(GetSelIdx(choiceCtrl.Key))->GetText();
			if (option.StartsWith(L"(auto)"))
			{
				existingChoices.Remove(choiceCtrl.Value);
			}
		}
		String choiceName;
		if (comboBoxChoiceNames.TryGetValue(cmb, choiceName))
		{
			existingChoices.Remove(choiceName);
			auto selText = cmb->GetTextItem(GetSelIdx(cmb))->GetText();
			if (!selText.StartsWith(L"(auto)"))
				existingChoices[choiceName] = Spire::Compiler::ShaderChoiceValue::Parse(selText);
		}
		auto choices = choiceControl->GetChoices(currentShaderName, existingChoices);
		for (auto & choice : choices)
		{
			GraphicsUI::ComboBox * cmbCtrl = nullptr;
			if (choiceComboBoxes.TryGetValue(choice.ChoiceName, cmbCtrl))
			{
				Spire::Compiler::ShaderChoiceValue currentSelection;
				existingChoices.TryGetValue(choice.ChoiceName, currentSelection);
				if (!choice.Options.Contains(currentSelection))
				{
					existingChoices.Remove(choice.ChoiceName);
					cmbCtrl->SetSelectedIndex(0);
				}
			}
		}
	}
	void ChoiceForm::Recompile()
	{
		SetCursor(LoadCursor(NULL, IDC_WAIT));
		
		auto schedule = GenerateSchedule(existingChoices, additionalAttribs);
		if (currentShaderName.Length())
			choiceControl->RecompileShader(currentShaderName, schedule);
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		ShaderChanged.Invoke(currentShaderName);
	}
	int ChoiceForm::AutotuneHelper(HashSet<String> & selectedChoices, EnumerableDictionary<String, Spire::Compiler::ShaderChoiceValue>& currentChoices, float timeBudget, int pipeline, bool countOnly)
	{
		auto choices = choiceControl->GetChoices(currentShaderName, currentChoices);
		List<Spire::Compiler::ShaderChoice> filteredChoices;
		for (auto & choice : choices)
			if (!currentChoices.ContainsKey(choice.ChoiceName))
				filteredChoices.Add(choice);
		choices = _Move(filteredChoices);
		// find first non-trivial choice
		Spire::Compiler::ShaderChoice * currentChoice = nullptr;
		for (auto & choice : choices)
		{
			if (choice.Options.Count() > 1 && selectedChoices.Contains(choice.ChoiceName))
			{
				currentChoice = &choice;
				break;
			}
		}
		if (currentChoice)
		{
			int count = 0;
			for (auto & opt : currentChoice->Options)
			{
				if (availableChoices[pipeline].Contains(opt.WorldName))
				{
					EnumerableDictionary<String, Spire::Compiler::ShaderChoiceValue> newChoices = currentChoices;
					newChoices[currentChoice->ChoiceName] = opt;
					count += AutotuneHelper(selectedChoices, newChoices, timeBudget, pipeline, countOnly);
				}
			}
			return count;
		}
		else
		{
			if (!countOnly)
			{
				printf("Testing shader variant...");
				// compile and evaluate
				auto schedule = GenerateSchedule(currentChoices, EnumerableDictionary<String, EnumerableDictionary<String, String>>());
				choiceControl->RecompileShader(currentShaderName, schedule);
				// render 1000 frames and measure time
				float minTime = 1e30f;
				for (int f = 0; f < 5; f++)
				{
					auto timeP = CoreLib::Diagnostics::PerformanceCounter::Start();
					for (int i = 0; i < 10; i++)
						choiceControl->RenderFrame();
					auto time = (float)CoreLib::Diagnostics::PerformanceCounter::ToSeconds(CoreLib::Diagnostics::PerformanceCounter::End(timeP)) * 100.0f;
					if (time < minTime)
						minTime = time;
				}
				auto frameData = ReadFrameData(choiceControl->RenderFrame());
				float error = MeasureError(referenceFrame, frameData);
				float value = 0;
				if (minTime < timeBudget)
					value = error;
				else
					value = 20.0f + minTime;
				if (value < currentBestValue)
				{
					currentBestValue = value;
					currentBestChoices = currentChoices;
				}
				autotuningLog << minTime << L"," << error<<EndLine;
				printf("%f ms, error: %f\n", minTime, error);
			}
			return 1;
		}
	}
	void ChoiceForm::Autotune(float timeBudget)
	{
		bool countOnly = false;
		ResetButton_Clicked(nullptr);
		referenceFrame = ReadFrameData(choiceControl->RenderFrame());
		EnumerableDictionary<String, Spire::Compiler::ShaderChoiceValue> currentChoices;
		currentBestValue = 1e30f;
		currentBestChoices = currentChoices;
		HashSet<String> selectedChoices;
		for (auto & chk : choiceCheckBoxes)
			if (chk.Value->Checked)
				selectedChoices.Add(chk.Key);
		autotuningLog.Clear();
		auto startTimePoint = PerformanceCounter::Start();
		int count = AutotuneHelper(selectedChoices, currentChoices, timeBudget, GetSelIdx(pipelineBox), countOnly);
		float time = PerformanceCounter::EndSeconds(startTimePoint);
		autotuningLog << L"time: " << time << EndLine;
		printf("variant count: %d\ntime: %f\n", count, time);
		File::WriteAllText(L"autotuneLog.txt", autotuningLog.ToString());
		if (!countOnly)
		{
			existingChoices = currentBestChoices;
			disableChoiceChangeCapture = true;
			for (auto & choice : existingChoices)
			{
				auto cmb = choiceComboBoxes[choice.Key].GetValue();
				int idxToSelect = 0;
				for (int i = 0; i < cmb->Items.Count(); i++)
					if (cmb->GetTextItem(i)->GetText() == choice.Value.ToString())
					{
						idxToSelect = i;
						break;
					}
				cmb->SetSelectedIndex(idxToSelect);
			}
			disableChoiceChangeCapture = false;
			Recompile();
		}
			
	}
	void ChoiceForm::AutotuneTex()
	{
		additionalAttribs.Clear();
		auto texs = choiceControl->GetPrecomputedTextures(currentShaderName);
		auto choices = choiceControl->GetChoices(currentShaderName, EnumerableDictionary<String, Spire::Compiler::ShaderChoiceValue>());

		for (auto & t : texs)
		{
			String choiceName;
			String searchKey = L"." + t.Key;
			for (auto & choice : choices)
				if (choice.ChoiceName.EndsWith(searchKey))
					choiceName = choice.ChoiceName;
			if (choiceName.Length() == 0)
				break;
			int w, h;
			t.Value.GetSize(w, h);
			List<float> data;
			data.SetSize(w*h*4);
			t.Value.GetComponents();
			for (auto & pix : data) pix = 0.0f;
			t.Value.GetData(0, DataType::Float4, data.Buffer(), w * h);
			bool isNormal = true, isColor = true;
			for (auto & pix : data)
			{
				if (!isNormal && !isColor)
					break;
				if (pix > 1.001f)
				{
					isColor = false;
					isNormal = false;
				}
				if (pix < -0.001)
				{
					isColor = false;
				}
				if (pix < -1.001)
				{
					isNormal = false;
				}
			}
			printf("PrecomputedTex %s: N=%d, C=%d\n", t.Key.ToMultiByteString(), isNormal?1:0, isColor?1:0);
				
			if (isColor)
			{
				EnumerableDictionary<String, String> attribs;
				attribs[L"Storage"] = L"RGBA8";
				additionalAttribs[choiceName] = attribs;
			}
			else if (isNormal)
			{
				EnumerableDictionary<String, String> attribs;
				attribs[L"Normal"] = L"1";
				additionalAttribs[choiceName] = attribs;
			}
		}
		Recompile();
	}
	void ChoiceForm::AutotuneTexButton_Clicked(GraphicsUI::UI_Base *)
	{
		if (currentShaderName.Length())
		{
			AutotuneTex();
		}
	}

	void ChoiceForm::SaveScheduleButton_Clicked(GraphicsUI::UI_Base *)
	{
		auto schedule = GenerateSchedule(existingChoices, additionalAttribs);
		FileDialog saveDlg(this->ownerForm);
		saveDlg.DefaultEXT = L"txt";
		saveDlg.Filter = L"Text File|*.txt";
		if (saveDlg.ShowSave())
		{
			File::WriteAllText(saveDlg.FileName, schedule);
		}
	}
	void ChoiceForm::AutotuneButton_Clicked(GraphicsUI::UI_Base *)
	{
		if (currentShaderName.Length())
		{
			Autotune((float)StringToDouble(timeBudgetTextBox->GetText()));
		}
	}
	void ChoiceForm::ResetButton_Clicked(GraphicsUI::UI_Base *)
	{
		disableChoiceChangeCapture = true;
		for (auto & box : choiceComboBoxes)
			box.Value->SetSelectedIndex(0);
		disableChoiceChangeCapture = false;
		existingChoices.Clear();
		if (autoRecompileCheckBox->Checked)
			Recompile();
	}
	void ChoiceForm::SelectedShaderChanged(GraphicsUI::UI_Base *)
	{
		SetCursor(LoadCursor(NULL, IDC_WAIT));
		int idx = GetSelIdx(shaderBox);
		if (idx != -1)
		{
			UpdateChoicePanel(shaderBox->GetTextItem(idx)->GetText());
			ShaderChanged(shaderBox->GetTextItem(idx)->GetText());
		}
		SetCursor(LoadCursor(NULL, IDC_ARROW));
	}
	void ChoiceForm::ChoiceComboBox_Changed(GraphicsUI::UI_Base *obj)
	{
		if (!disableChoiceChangeCapture)
		{
			disableChoiceChangeCapture = true;
			UpdateChoiceOptions((GraphicsUI::ComboBox*)obj);
			disableChoiceChangeCapture = false;
			if (autoRecompileCheckBox->Checked)
			{
				Recompile();
			}
		}
	}
	void ChoiceForm::PipelineBox_Changed(GraphicsUI::UI_Base *)
	{
		SelectedShaderChanged(nullptr);
		if (autoRecompileCheckBox->Checked)
			Recompile();
	}
	void ChoiceForm::ApplyButton_Clicked(GraphicsUI::UI_Base *)
	{
		Recompile();
	}
	void ChoiceForm::ChoiceForm_OnResize(GraphicsUI::UI_Base *)
	{
		shaderBox->Posit(10, 50, 100, GetClientHeight() - 60);
#ifdef ENABLE_AUTO_TUNE
		scrollPanel->Posit(120, 90, GetClientWidth() - 120, GetClientHeight() - 100);
#else
		scrollPanel->Posit(120, 50, GetClientWidth() - 120, GetClientHeight() - 60);
#endif
		int scWidth = scrollPanel->GetClientWidth();
		int cmbLeft = scWidth;
		for (auto & cmb : choiceComboBoxes)
		{
			int cmbW = cmb.Value->GetWidth();
			cmbLeft = scWidth - 10 - cmbW;
			cmb.Value->Left = scWidth - 10 - cmbW;
		}
		for (int i = 0; i < scrollPanel->GetChildren().Count(); i++)
		{
			auto child = scrollPanel->GetChildren()[i];
			if (auto lbl = dynamic_cast<GraphicsUI::CheckBox*>(child.Ptr()))
			{
				lbl->SetWidth(cmbLeft - 5);
			}
		}
	}
	void ChoiceForm::Update()
	{
		shaderBox->Clear();
		auto shaders = choiceControl->GetShaders();
		for (auto & shader : shaders)
			shaderBox->AddTextItem(shader);
		if (shaders.Count())
			UpdateChoicePanel(shaders.First());
		else
			UpdateChoicePanel(L"");
		if (shaders.Count())
			ShaderChanged.Invoke(shaders.First());
	}
}
