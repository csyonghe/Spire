#include "Skeleton.h"

namespace GameEngine
{
	using namespace CoreLib::IO;

	void Skeleton::SaveToStream(CoreLib::IO::Stream * stream)
	{
		BinaryWriter writer(stream);
		writer.Write(this->Bones.Count());
		for (int i = 0; i < Bones.Count(); i++)
		{
			writer.Write(Bones[i].Name);
			writer.Write(Bones[i].ParentId);
			writer.Write(InversePose[i]);
			writer.Write(Bones[i].BindPose);
		}
		writer.ReleaseStream();
	}

	void Skeleton::LoadFromStream(CoreLib::IO::Stream * stream)
	{
		BinaryReader reader(stream);
		int boneCount = reader.ReadInt32();
		Bones.SetSize(boneCount);
		InversePose.SetSize(boneCount);
		BoneMapping.Clear();
		for (int i = 0; i < Bones.Count(); i++)
		{
			reader.Read(Bones[i].Name);
			reader.Read(Bones[i].ParentId);
			reader.Read(InversePose[i]);
			reader.Read(Bones[i].BindPose);
			BoneMapping[Bones[i].Name] = i;
		}
		reader.ReleaseStream();
	}

	void Skeleton::SaveToFile(const String & file)
	{
		RefPtr<FileStream> stream = new FileStream(file, FileMode::Create);
		SaveToStream(stream.Ptr());
		stream->Close();
	}

	void Skeleton::LoadFromFile(const String & file)
	{
		RefPtr<FileStream> stream = new FileStream(file, FileMode::Open);
		LoadFromStream(stream.Ptr());
		stream->Close();
	}
	void SkeletalAnimation::SaveToStream(CoreLib::IO::Stream * stream)
	{
		BinaryWriter writer(stream);
		writer.Write(Name);
		writer.Write(Speed);
		writer.Write(Duration);
		writer.Write(Reserved);
		writer.Write(Channels.Count());
		for (int i = 0; i < Channels.Count(); i++)
		{
			writer.Write(Channels[i].BoneName);
			writer.Write(Channels[i].KeyFrames);
		}
		writer.ReleaseStream();
	}
	void SkeletalAnimation::LoadFromStream(CoreLib::IO::Stream * stream)
	{
		BinaryReader reader(stream);
		reader.Read(Name);
		reader.Read(Speed);
		reader.Read(Duration);
		reader.Read(Reserved);
		int channelCount = reader.ReadInt32();
		Channels.SetSize(channelCount);
		for (int i = 0; i < Channels.Count(); i++)
		{
			reader.Read(Channels[i].BoneName);
			reader.Read(Channels[i].KeyFrames);
		}
		reader.ReleaseStream();
	}
	void SkeletalAnimation::SaveToFile(const CoreLib::String & filename)
	{
		RefPtr<FileStream> stream = new FileStream(filename, FileMode::Create);
		SaveToStream(stream.Ptr());
		stream->Close();
	}
	void SkeletalAnimation::LoadFromFile(const CoreLib::String & filename)
	{
		RefPtr<FileStream> stream = new FileStream(filename, FileMode::Open);
		LoadFromStream(stream.Ptr());
		stream->Close();
	}
}
