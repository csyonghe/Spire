#ifndef DEMO_RECORDING_H
#define DEMO_RECORDING_H

#include "CoreLib/Basic.h"
#include "CoreLib/VectorMath.h"
#include "CoreLib/LibIO.h"

using namespace CoreLib::Basic;
using namespace CoreLib::IO;
using namespace VectorMath;

class KeyFrame
{
public:
	float Time;
	float Alpha, Beta;
	Vec3 Position;
};

class DemoRecording
{
private:
	float time;
public:
	DemoRecording()
	{
		time = 0.0f;
	}
	List<KeyFrame> Frames;
	void AddKeyFrame(float dTime, float alpha, float beta, const Vec3 & position)
	{
		KeyFrame f;
		f.Time = time + dTime;
		time += dTime;
		f.Alpha = alpha;
		f.Beta = beta;
		f.Position = position;
		Frames.Add(f);
	}

	void SaveToFile(String fileName)
	{
		BinaryWriter writer(new FileStream(fileName, FileMode::Create));
		writer.Write(time);
		writer.Write(Frames.Count());
		writer.Write(Frames.Buffer(), Frames.Count());
	}

	void LoadFromFile(String fileName)
	{
		BinaryReader reader(new FileStream(fileName, FileMode::Open));
		time = reader.ReadFloat();
		Frames.SetSize(reader.ReadInt32());
		reader.Read(Frames.Buffer(), Frames.Count());
	}
};

class DemoRecordingPlayback
{
private:
	DemoRecording * demo;
	float time;
	int frameId;
public:
	DemoRecordingPlayback(DemoRecording * demo)
		:demo(demo)
	{
		frameId = 0;
		time = 0.0f;
	}
	bool AdvanceFrame(float dTime)
	{
		time += dTime;
		while (frameId + 1 < demo->Frames.Count())
		{
			if (demo->Frames[frameId+1].Time <= time)
				frameId++;
			else
				break;
		}
		return frameId + 1 < demo->Frames.Count();
	}
	KeyFrame GetCurrentFrame()
	{
		float t0 = time - demo->Frames[frameId].Time;
		float tt = demo->Frames[frameId+1].Time - demo->Frames[frameId].Time;
		float invT = t0/tt;
		float t = 1.0f - invT;
		KeyFrame f;
		f.Time = time;
		f.Alpha = demo->Frames[frameId].Alpha*t + invT*demo->Frames[frameId+1].Alpha;
		f.Beta = demo->Frames[frameId].Beta*t + invT*demo->Frames[frameId+1].Beta;
		f.Position = demo->Frames[frameId].Position*t + demo->Frames[frameId+1].Position*invT;
		return f;
	}
};

class CameraPoint
{
public:
	Vec3 Position;
	Vec3 ViewPoint;
};

class CameraCurve
{
private:
	bool initialized;
public:
	List<CameraPoint> Points;
	List<float> Distances;
	float TotalDistance;
	CameraCurve()
	{
		initialized = false;
		TotalDistance = 0.0f;
	}
	void Initialize()
	{
		initialized = true;
		Distances.Clear();
		TotalDistance = 0.0f;
		Distances.Add(0.0f);
		for (int i = 1; i < Points.Count(); i++)
		{
			float dist = (Points[i].Position - Points[i - 1].Position).Length();
			TotalDistance += dist;
			Distances.Add(TotalDistance);
		}
	}
	Camera ExtractCamera(float t) // t from 0 to TotalDistance
	{
		if (!initialized)
			throw InvalidOperationException(L"Camera curve must be initialized before interpolating.");
		Camera rs;
		for (int i = 0; i < Points.Count() - 1; i++)
		{
			if (Distances[i] <= t && t < Distances[i + 1])
			{
				float it = (t - Distances[i]) / (Distances[i + 1] - Distances[i]);
				CameraPoint cp;
				if (i == 0 || i == Points.Count() - 2)
				{
					// lerp
					cp.Position = Points[i].Position * (1.0f - it) + Points[i + 1].Position * it;
					cp.ViewPoint = Points[i].ViewPoint * (1.0f - it) + Points[i + 1].ViewPoint * it;
				}
				else
				{
					// catmull interp
					cp.Position = CatmullInterpolate(Points[i - 1].Position, Points[i].Position, Points[i + 1].Position, Points[i + 2].Position, it);
					cp.ViewPoint = CatmullInterpolate(Points[i - 1].ViewPoint, Points[i].ViewPoint, Points[i + 1].ViewPoint, Points[i + 2].ViewPoint, it);
				}
				auto dir = cp.ViewPoint - cp.Position;
				rs.pos = cp.Position;
				Vec3::Normalize(rs.dir, dir);
				rs.beta = asin(rs.dir.y);
				rs.alpha = atan2(dir.x / cos(rs.beta), dir.z / cos(rs.beta));
				/*Vec3 ndir = Vec3((float)sin(rs.alpha)*cos(rs.beta),
					(float)sin(rs.beta),
					(float)cos(rs.alpha)*cos(rs.beta));*/
				return rs;
			}
		}
		return rs;
	}
	void AddPoint(const Vec3 & pos, const Vec3 & viewPoint)
	{
		CameraPoint cp;
		cp.Position = pos;
		cp.ViewPoint = viewPoint;
		Points.Add(cp);
	}
	void RemoveLastPoint()
	{
		if (Points.Count())
			Points.RemoveAt(Points.Count() - 1);
	}
	void Save(const String & fileName)
	{
		BinaryWriter writer(new FileStream(fileName, FileMode::Create));
		writer.Write(Points.Count());
		writer.Write(Points.Buffer(), Points.Count());
		Initialize();
	}
	void Load(const String & fileName)
	{
		BinaryReader reader(new FileStream(fileName, FileMode::Open));
		int count = reader.ReadInt32();
		Points.SetSize(count);
		reader.Read(Points.Buffer(), count);
	}
};


#endif