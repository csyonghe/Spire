#include "CoreLib/Basic.h"
#include "CoreLib/LibIO.h"
#include "Skeleton.h"
#include "Mesh.h"
#include "WinForm/WinButtons.h"
#include "WinForm/WinCommonDlg.h"
#include "WinForm/WinForm.h"
#include "WinForm/WinApp.h"
#include "WinForm/WinListBox.h"
#include "WinForm/WinTextBox.h"

#include "BvhFile.h"

using namespace CoreLib;
using namespace CoreLib::IO;
using namespace CoreLib::Graphics;
using namespace GameEngine;
using namespace VectorMath;
using namespace CoreLib::WinForm;
using namespace GameEngine::Tools;


class ExportArguments
{
public:
	String FileName;
	String SkeletonFileName;
	String EulerOrder = L"ZXY";
	bool ExportSkeleton = true;
	bool ExportMesh = false;
	bool ExportAnimation = true;
	bool FlipYZ = true;
};

Quaternion FlipYZ(Quaternion q)
{
	return Quaternion::FromAxisAngle(Vec3::Create(1.0f, 0.0f, 0.0f), -Math::Pi*0.5f) * q * Quaternion::FromAxisAngle(Vec3::Create(1.0f, 0.0f, 0.0f), Math::Pi*0.5f);
}

Vec3 FlipYZ(const Vec3 & v)
{
	return Vec3::Create(v.x, v.z, -v.y);
}

Matrix4 FlipYZ(const Matrix4 & v)
{
	Matrix4 rs = v;
	for (int i = 0; i < 4; i++)
	{
		rs.m[1][i] = v.m[2][i];
		rs.m[2][i] = -v.m[1][i];
	}
	for (int i = 0; i < 4; i++)
	{
		Swap(rs.m[i][1], rs.m[i][2]);
		rs.m[i][2] = -rs.m[i][2];
	}
	return rs;
}

template<typename TFunc>
void TraverseBvhJoints(BvhJoint * joint, const TFunc & f)
{
	f(joint);
	for (auto & child : joint->SubJoints)
		TraverseBvhJoints(joint, f);
}

void TraverseBvhJoints(BvhJoint * parent, BvhJoint * joint, Skeleton & skeleton)
{
	Bone b;
	b.Name = joint->Name;
	b.BindPose.Translation = joint->Offset;
	b.ParentId = parent ? skeleton.BoneMapping[parent->Name]() : -1;
	skeleton.BoneMapping[joint->Name] = skeleton.Bones.Count();
	skeleton.Bones.Add(b);
	for (auto & child : joint->SubJoints)
		TraverseBvhJoints(joint, child.Ptr(), skeleton);
}

void FindBBox(BBox & bbox, BvhJoint * joint)
{
	bbox.Union(joint->Offset);
	for (auto & child : joint->SubJoints)
		FindBBox(bbox, child.Ptr());
}

void GatherNonTrivialBvhJoints(List<BvhJoint*> & list, BvhJoint * joint)
{
	if (joint->Channels.Count())
		list.Add(joint);
	for (auto & child : joint->SubJoints)
		GatherNonTrivialBvhJoints(list, child.Ptr());
}

void FlipKeyFrame(BoneTransformation & kf)
{
	Matrix4 transform = kf.ToMatrix();
	Matrix4 rotX90, rotXNeg90;
	Matrix4::RotationX(rotX90, Math::Pi * 0.5f);
	Matrix4::RotationX(rotXNeg90, -Math::Pi * 0.5f);

	Matrix4::Multiply(transform, transform, rotX90);
	Matrix4::Multiply(transform, rotXNeg90, transform);
	
	kf.Rotation = Quaternion::FromMatrix(transform.GetMatrix3());
	kf.Rotation *= 1.0f / kf.Rotation.Length();
	kf.Translation = Vec3::Create(transform.values[12], transform.values[13], transform.values[14]);

	Swap(kf.Scale.y, kf.Scale.z);
}

void FlipKeyFrameCoordinateSystem(BoneTransformation & kf)
{
	Matrix4 transform = kf.ToMatrix();
	Matrix4 flipMatrix;
	Matrix4::Scale(flipMatrix, 1.0f, 1.0f, -1.0f);
	Matrix4::Multiply(transform, transform, flipMatrix);
	Matrix4::Multiply(transform, flipMatrix, transform);

	kf.Rotation = Quaternion::FromMatrix(transform.GetMatrix3());
	kf.Rotation *= 1.0f / kf.Rotation.Length();
	kf.Translation = Vec3::Create(transform.values[12], transform.values[13], transform.values[14]);
}

void Export(const ExportArguments & args)
{
	BvhFile file = BvhFile::FromFile(args.FileName);
	Skeleton skeleton;

	if (file.Hierarchy)
	{
		TraverseBvhJoints(nullptr, file.Hierarchy.Ptr(), skeleton);
		// compute inverse binding pose
		skeleton.InversePose.SetSize(skeleton.Bones.Count());
		for (int i = 0; i < skeleton.InversePose.Count(); i++)
		{
			auto & bone = skeleton.Bones[i];
			if (args.FlipYZ)
			{
				bone.BindPose.Translation = FlipYZ(bone.BindPose.Translation);
			}
			if (bone.ParentId != -1)
				Matrix4::Multiply(skeleton.InversePose[i], skeleton.InversePose[bone.ParentId], bone.BindPose.ToMatrix());
			else
				skeleton.InversePose[i] = bone.BindPose.ToMatrix();

		}
		for (auto & bone : skeleton.InversePose)
		{
			bone.Inverse(bone);
		}
		if (args.ExportSkeleton)
			skeleton.SaveToFile(Path::ReplaceExt(args.FileName, L"skeleton"));
	}

	if (args.SkeletonFileName.Length())
		skeleton.LoadFromFile(args.SkeletonFileName);
	
	if (args.ExportAnimation)
	{
		List<BvhJoint*> joints;
		GatherNonTrivialBvhJoints(joints, file.Hierarchy.Ptr());
		SkeletalAnimation anim;
		anim.Speed = 1.0f;
		for (auto & joint : joints)
		{
			AnimationChannel ch;
			int boneId = skeleton.BoneMapping[joint->Name]();
			ch.BoneId = boneId;
			ch.BoneName = joint->Name;
			anim.Channels.Add(ch);
		}
		int frameId = 0;
		int ptr = 0;
		auto readData = [&]() 
		{
			if (ptr < file.FrameData.Count())
			{
				float data = file.FrameData[ptr];
				ptr++;
				return data;
			}
			else
			{
				throw InvalidOperationException(L"MOTION data size does not match HIERARCHY channels declaration.");
			}
		};
		
		while (ptr < file.FrameData.Count())
		{
			anim.Duration = file.FrameDuration * frameId;
			for (auto joint : joints)
			{
				int boneId = skeleton.BoneMapping[joint->Name]();
				AnimationKeyFrame keyFrame;
				float rotX = 0.0f, rotY = 0.0f, rotZ = 0.0f;
				bool hasTranslation = false;
				for (auto & c : joint->Channels)
				{
					switch (c)
					{
					case ChannelType::XPos:
						keyFrame.Transform.Translation.x = readData();
						hasTranslation = true;
						break;
					case ChannelType::YPos:
						keyFrame.Transform.Translation.y = readData();
						hasTranslation = true;
						break;
					case ChannelType::ZPos:
						keyFrame.Transform.Translation.z = readData();
						hasTranslation = true;
						break;
					case ChannelType::XScale:
						keyFrame.Transform.Scale.x = readData();
						break;
					case ChannelType::YScale:
						keyFrame.Transform.Scale.y = readData();
						break;
					case ChannelType::ZScale:
						keyFrame.Transform.Scale.z = readData();
						break;
					case ChannelType::XRot:
						rotX = readData();
						break;
					case ChannelType::YRot:
						rotY = readData();
						break;
					case ChannelType::ZRot:
						rotZ = readData();
						break;
					}
				}
				Matrix4 xMat, yMat, zMat, rot;
				Matrix4::RotationX(xMat, rotX * (Math::Pi / 180.0f));
				Matrix4::RotationY(yMat, rotY * (Math::Pi / 180.0f));
				Matrix4::RotationZ(zMat, rotZ * (Math::Pi / 180.0f));
				if (args.EulerOrder == L"ZXY")
				{
					Matrix4::Multiply(rot, xMat, zMat);
					Matrix4::Multiply(rot, yMat, rot);
				}
				else if (args.EulerOrder == L"ZYX")
				{
					Matrix4::Multiply(rot, yMat, zMat);
					Matrix4::Multiply(rot, xMat, rot);
				}
				else if (args.EulerOrder == L"YXZ")
				{
					Matrix4::Multiply(rot, xMat, yMat);
					Matrix4::Multiply(rot, zMat, rot);
				}
				else if (args.EulerOrder == L"XYZ")
				{
					Matrix4::Multiply(rot, yMat, xMat);
					Matrix4::Multiply(rot, zMat, rot);
				}
				else if (args.EulerOrder == L"XZY")
				{
					Matrix4::Multiply(rot, zMat, xMat);
					Matrix4::Multiply(rot, yMat, rot);
				}
				else if (args.EulerOrder == L"YZX")
				{
					Matrix4::Multiply(rot, zMat, yMat);
					Matrix4::Multiply(rot, xMat, rot);
				}
				keyFrame.Transform.Rotation = Quaternion::FromMatrix(rot.GetMatrix3());
				if (args.FlipYZ)
				{
					FlipKeyFrame(keyFrame.Transform);
				}
				if (!hasTranslation)
					keyFrame.Transform.Translation = skeleton.Bones[boneId].BindPose.Translation;
				keyFrame.Time = file.FrameDuration * frameId;
				anim.Channels[boneId].KeyFrames.Add(keyFrame);
			}
			frameId++;
		}
		anim.SaveToFile(Path::ReplaceExt(args.FileName, L"anim"));
	}
	if (args.ExportMesh)
	{
		BBox bbox;
		bbox.Init();
		FindBBox(bbox, file.Hierarchy.Ptr());
		Mesh mesh;
		mesh.FromSkeleton(&skeleton, (bbox.Max-bbox.Min).Length() * 0.08f);
		mesh.SaveToFile(Path::ReplaceExt(args.FileName, L"mesh"));
	}
}

class MocapConverterForm : public Form
{
private:
	RefPtr<Button> btnSelectFiles;
	RefPtr<CheckBox> chkFlipYZ, chkCreateMesh, chkConvertLHC;
	RefPtr<ComboBox> cmbEuler;
	RefPtr<Label> lblSkeleton;
	RefPtr<TextBox> txtSkeletonFile;
	RefPtr<Button> btnSelectSkeletonFile;
public:
	MocapConverterForm()
	{
		SetText(L"Convert Mocap Data");
		SetClientWidth(420);
		SetClientHeight(210);
		CenterScreen();
		SetMaximizeBox(false);
		SetBorder(fbFixedDialog);
		chkFlipYZ = new CheckBox(this);
		chkFlipYZ->SetPosition(30, 20, 120, 25);
		chkFlipYZ->SetText(L"Flip YZ");
		chkFlipYZ->SetChecked(ExportArguments().FlipYZ);
		chkCreateMesh = new CheckBox(this);
		chkCreateMesh->SetPosition(30, 50, 120, 25);
		chkCreateMesh->SetText(L"Create Mesh");
		chkCreateMesh->SetChecked(ExportArguments().ExportMesh);
		cmbEuler = new ComboBox(this);
		cmbEuler->AddItem(L"Euler Angle Order");
		cmbEuler->AddItem(L"ZXY");
		cmbEuler->AddItem(L"ZYX");
		cmbEuler->AddItem(L"YXZ");
		cmbEuler->AddItem(L"XYZ");
		cmbEuler->AddItem(L"XZY");
		cmbEuler->AddItem(L"YZX");

		cmbEuler->SetPosition(30, 80, 120, 30);
		cmbEuler->SetSelectionIndex(0);

		lblSkeleton = new Label(this);
		lblSkeleton->SetText(L"Retarget Skeleton:");
		lblSkeleton->SetPosition(30, 113, 120, 30);
		txtSkeletonFile = new TextBox(this);
		txtSkeletonFile->SetText(L"");
		txtSkeletonFile->SetPosition(150, 110, 140, 25);
		btnSelectSkeletonFile = new Button(this);
		btnSelectSkeletonFile->SetText(L"Browse");
		btnSelectSkeletonFile->SetPosition(300, 110, 80, 25);

		btnSelectSkeletonFile->OnClick.Bind([this](Object*, EventArgs)
		{
			FileDialog dlg(this);
			dlg.Filter = L"Skeleton|*.skeleton";
			if (dlg.ShowOpen())
				txtSkeletonFile->SetText(dlg.FileName);
		});

		btnSelectFiles = new Button(this);
		btnSelectFiles->SetPosition(90, 150, 120, 30);
		btnSelectFiles->SetText(L"Select Files");
		btnSelectFiles->OnClick.Bind([=](Object*, EventArgs)
		{
			try
			{
				FileDialog dlg(this);
				dlg.MultiSelect = true;
				if (dlg.ShowOpen())
				{
					for (auto file : dlg.FileNames)
					{
						ExportArguments args;
						args.FlipYZ = chkFlipYZ->GetChecked();
						args.ExportMesh = chkCreateMesh->GetChecked();
						args.FileName = file;
						args.SkeletonFileName = txtSkeletonFile->GetText();
						if (cmbEuler->GetSelectionIndex() > 0)
							args.EulerOrder = cmbEuler->GetItem(cmbEuler->GetSelectionIndex());
						SetCursor(LoadCursor(NULL, IDC_WAIT));
						Export(args);
						SetCursor(LoadCursor(NULL, IDC_ARROW));
					}
				}
			}
			catch (const Exception & e)
			{
				MessageBox(String("Error: ") + e.Message, L"Error", MB_ICONEXCLAMATION);
			}
		});
	}
};

int wmain(int argc, const wchar_t** argv)
{
	if (argc > 1)
	{
		try
		{
			ExportArguments args;
			args.FlipYZ = false;
			args.ExportMesh = false;
			args.FileName = String(argv[1]);
			for (int i = 2; i < argc; i++)
				if (String(argv[i]) == L"-flipyz")
					args.FlipYZ = true;
				else if (String(argv[i]) == L"-mesh")
					args.ExportMesh = true;
				else if (String(argv[i]) == L"-euler" && i < argc - 1)
					args.EulerOrder = argv[i + 1];
			Export(args);
		}
		catch (const Exception & e)
		{
			printf("Error: %S\n", e.Message.Buffer());
			return 1;
		}
	}
	else
	{
		Application::Init();
		Application::Run(new MocapConverterForm());
		Application::Dispose();
	}
	_CrtDumpMemoryLeaks();
	return 0;
}