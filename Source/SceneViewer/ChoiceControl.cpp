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
	ChoiceForm::ChoiceForm(IChoiceControl * pChoiceControl)
	{
		this->choiceControl = pChoiceControl;
		SetText("Choice Control");
		SetClientWidth(430);
		SetClientHeight(640);
		InitUI();
		OnResized.Bind(this, &ChoiceForm::ChoiceForm_OnResize);
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
		pipelineBox = new ComboBox(this);
		pipelineBox->AddItem(L"Standard");
		pipelineBox->AddItem(L"Precompute");
		pipelineBox->AddItem(L"MultiRate");
		pipelineBox->AddItem(L"MultiRateObjSpace");
		pipelineBox->SetSelectionIndex(0);
		pipelineBox->SelectionChanged.Bind(this, &ChoiceForm::PipelineBox_Changed);
		shaderBox = new ListBox(this);
		shaderBox->SetPosition(10, 50, 100, GetClientHeight() - 60);
		shaderBox->SelectionChanged.Bind(this, &ChoiceForm::SelectedShaderChanged);
		applyButton = new Button(this);
		applyButton->SetPosition(120, 10, 80, 30);
		applyButton->SetText(L"Re&compile");
		applyButton->OnClick.Bind(this, &ChoiceForm::ApplyButton_Clicked);
		resetButton = new Button(this);
		resetButton->SetPosition(210, 10, 80, 30);
		resetButton->SetText(L"&Reset");
		resetButton->OnClick.Bind(this, &ChoiceForm::ResetButton_Clicked);
		autoRecompileCheckBox = new CheckBox(this);
		autoRecompileCheckBox->SetText(L"&Auto Recompile");
		autoRecompileCheckBox->SetPosition(310, 14, 110, 25);
		autoRecompileCheckBox->SetChecked(true);
#ifdef ENABLE_AUTO_TUNE
		timeBudgetTextBox = new TextBox(this);
		timeBudgetTextBox->SetText(L"10");
		timeBudgetTextBox->SetPosition(120, 52, 80, 25);
		autoTuneButton = new Button(this);
		autoTuneButton->SetPosition(210, 50, 80, 30);
		autoTuneButton->SetText(L"Auto&tune");
		autoTuneButton->OnClick.Bind(this, &ChoiceForm::AutotuneButton_Clicked);
		autoTuneTexButton = new Button(this);
		autoTuneTexButton->SetPosition(300, 50, 80, 30);
		autoTuneTexButton->SetText(L"Tune Te&xture");
		autoTuneTexButton->OnClick.Bind(this, &ChoiceForm::AutotuneTexButton_Clicked);
		saveScheduleButton = new Button(this);
		saveScheduleButton->SetPosition(390, 50, 80, 30);
		saveScheduleButton->SetText(L"&Save");
		saveScheduleButton->OnClick.Bind(this, &ChoiceForm::SaveScheduleButton_Clicked);
#endif
		scrollPanel = new ScrollPanel(this);
		scrollPanel->SetPosition(120, 90, GetClientWidth() - 120, GetClientHeight() - 100);
		scrollPanel->SetScrollBar(ScrollBars::Vertical);
	}
	void ChoiceForm::UpdateChoicePanel(String shaderName)
	{
		scrollPanel->DisableRedraw();
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
			auto lbl = new CheckBox(scrollPanel.Ptr());
			lbl->SetText(choice.ChoiceName);
			lbl->BringToTop();
			lbl->SetPosition(0, line * 30, cmbLeft - 5, 25);
			scrollPanel->AddChild(lbl);
			choiceCheckBoxes.Add(choice.ChoiceName, lbl);
			auto cmb = new ComboBox(scrollPanel.Ptr());
			cmb->AddItem(L"(auto) " + choice.DefaultValue);
			for (auto & opt : choice.Options)
			{
				if (availableChoices[pipelineBox->GetSelectionIndex()].Contains(opt.WorldName))
					cmb->AddItem(opt.ToString());
			}
			cmb->BringToTop();
			comboBoxChoiceNames[cmb] = choice.ChoiceName;
			choiceComboBoxes[choice.ChoiceName] = cmb;
			cmb->SetSelectionIndex(0);
			cmb->SelectionChanged.Bind(this, &ChoiceForm::ChoiceComboBox_Changed);
			cmb->SetPosition(cmbLeft, line * 30, 130, 25);
			scrollPanel->AddChild(cmb);
			line++;
		}
		scrollPanel->EnableRedraw();
		ChoiceForm_OnResize(this, EventArgs());
	}
	void ChoiceForm::UpdateChoiceOptions(ComboBox * cmb)
	{
		for (auto & choiceCtrl : comboBoxChoiceNames)
		{
			String option = choiceCtrl.Key->GetItem(choiceCtrl.Key->GetSelectionIndex());
			if (option.StartsWith(L"(auto)"))
			{
				existingChoices.Remove(choiceCtrl.Value);
			}
		}
		String choiceName;
		if (comboBoxChoiceNames.TryGetValue(cmb, choiceName))
		{
			existingChoices.Remove(choiceName);
			auto selText = cmb->GetItem(cmb->GetSelectionIndex());
			if (!selText.StartsWith(L"(auto)"))
				existingChoices[choiceName] = Spire::Compiler::ShaderChoiceValue::Parse(selText);
		}
		auto choices = choiceControl->GetChoices(currentShaderName, existingChoices);
		for (auto & choice : choices)
		{
			ComboBox * cmbCtrl = nullptr;
			if (choiceComboBoxes.TryGetValue(choice.ChoiceName, cmbCtrl))
			{
				Spire::Compiler::ShaderChoiceValue currentSelection;
				existingChoices.TryGetValue(choice.ChoiceName, currentSelection);
				if (!choice.Options.Contains(currentSelection))
				{
					existingChoices.Remove(choice.ChoiceName);
					cmbCtrl->SetSelectionIndex(0);
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
		ResetButton_Clicked(nullptr, EventArgs());
		referenceFrame = ReadFrameData(choiceControl->RenderFrame());
		EnumerableDictionary<String, Spire::Compiler::ShaderChoiceValue> currentChoices;
		currentBestValue = 1e30f;
		currentBestChoices = currentChoices;
		HashSet<String> selectedChoices;
		for (auto & chk : choiceCheckBoxes)
			if (chk.Value->GetChecked())
				selectedChoices.Add(chk.Key);
		autotuningLog.Clear();
		auto startTimePoint = PerformanceCounter::Start();
		int count = AutotuneHelper(selectedChoices, currentChoices, timeBudget, pipelineBox->GetSelectionIndex(), countOnly);
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
				for (int i = 0; i < cmb->Count(); i++)
					if (cmb->GetItem(i) == choice.Value.ToString())
					{
						idxToSelect = i;
						break;
					}
				cmb->SetSelectionIndex(idxToSelect);
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
	void ChoiceForm::AutotuneTexButton_Clicked(Object *, EventArgs e)
	{
		if (currentShaderName.Length())
		{
			AutotuneTex();
		}
	}

	void ChoiceForm::SaveScheduleButton_Clicked(Object *, EventArgs e)
	{
		auto schedule = GenerateSchedule(existingChoices, additionalAttribs);
		FileDialog saveDlg(this);
		saveDlg.DefaultEXT = L"txt";
		saveDlg.Filter = L"Text File|*.txt";
		if (saveDlg.ShowSave())
		{
			File::WriteAllText(saveDlg.FileName, schedule);
		}
	}
	void ChoiceForm::AutotuneButton_Clicked(Object *, EventArgs e)
	{
		if (currentShaderName.Length())
		{
			Autotune((float)StringToDouble(timeBudgetTextBox->GetText()));
		}
	}
	void ChoiceForm::ResetButton_Clicked(Object * , EventArgs e)
	{
		disableChoiceChangeCapture = true;
		for (auto & box : choiceComboBoxes)
			box.Value->SetSelectionIndex(0);
		disableChoiceChangeCapture = false;
		existingChoices.Clear();
		if (autoRecompileCheckBox->GetChecked())
			Recompile();
	}
	void ChoiceForm::SelectedShaderChanged(Object *, EventArgs)
	{
		SetCursor(LoadCursor(NULL, IDC_WAIT));
		int idx = shaderBox->GetSelectionIndex();
		if (idx != -1)
		{
			UpdateChoicePanel(shaderBox->GetItem(idx));
			ShaderChanged(shaderBox->GetItem(idx));
		}
		SetCursor(LoadCursor(NULL, IDC_ARROW));
	}
	void ChoiceForm::ChoiceComboBox_Changed(Object * obj, EventArgs)
	{
		if (!disableChoiceChangeCapture)
		{
			disableChoiceChangeCapture = true;
			UpdateChoiceOptions((ComboBox*)obj);
			disableChoiceChangeCapture = false;
			if (autoRecompileCheckBox->GetChecked())
			{
				Recompile();
			}
		}
	}
	void ChoiceForm::PipelineBox_Changed(Object *, EventArgs)
	{
		SelectedShaderChanged(nullptr, EventArgs());
		if (autoRecompileCheckBox->GetChecked())
			Recompile();
	}
	void ChoiceForm::ApplyButton_Clicked(Object *, EventArgs)
	{
		Recompile();
	}
	void ChoiceForm::ChoiceForm_OnResize(Object *, EventArgs)
	{
		shaderBox->SetPosition(10, 50, 100, GetClientHeight() - 60);
		scrollPanel->DisableRedraw();
#ifdef ENABLE_AUTO_TUNE
		scrollPanel->SetPosition(120, 90, GetClientWidth() - 120, GetClientHeight() - 100);
#else
		scrollPanel->SetPosition(120, 50, GetClientWidth() - 120, GetClientHeight() - 60);
#endif
		int scWidth = scrollPanel->GetClientWidth();
		int cmbLeft = scWidth;
		for (auto & cmb : choiceComboBoxes)
		{
			int cmbW = cmb.Value->GetWidth();
			cmbLeft = scWidth - 10 - cmbW;
			cmb.Value->SetLeft(scWidth - 10 - cmbW);
		}
		for (int i = 0; i < scrollPanel->GetChildrenCount(); i++)
		{
			auto child = scrollPanel->GetChildren(i);
			if (auto lbl = dynamic_cast<CheckBox*>(child))
			{
				lbl->SetWidth(cmbLeft - 5);
			}
		}
		scrollPanel->EnableRedraw();
		scrollPanel->Invalidate();
	}
	void ChoiceForm::Update()
	{
		shaderBox->Clear();
		auto shaders = choiceControl->GetShaders();
		for (auto & shader : shaders)
			shaderBox->AddItem(shader);
		if (shaders.Count())
			UpdateChoicePanel(shaders.First());
		else
			UpdateChoicePanel(L"");
		if (shaders.Count())
			ShaderChanged.Invoke(shaders.First());
	}
}
