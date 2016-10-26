#include "BvhFile.h"
#include "CoreLib/Parser.h"
#include "CoreLib/LibIO.h"

using namespace CoreLib;
using namespace CoreLib::IO;
using namespace VectorMath;
using namespace CoreLib::Text;

namespace GameEngine
{
	namespace Tools
	{
		RefPtr<BvhJoint> ParseNode(Parser & p)
		{
			RefPtr<BvhJoint> result = new BvhJoint();
			if (p.LookAhead(L"ROOT") || p.LookAhead(L"JOINT"))
				p.ReadToken();
			else if (p.LookAhead(L"End"))
			{
				p.ReadToken();
				p.Read(L"Site");
				p.Read(L"{");
				p.Read(L"OFFSET");
				for (int i = 0; i < 3; i++)
					result->Offset[i] = (float)p.ReadDouble();
				p.Read(L"}");
				return result;
			}
			else
				throw TextFormatException(L"Invalid file format: expecting 'ROOT' or 'JOINT'.");
			result->Name = p.ReadToken().Str;
			p.Read(L"{");
			while (!p.LookAhead(L"}") && !p.IsEnd())
			{
				if (p.LookAhead(L"OFFSET"))
				{
					p.ReadToken();
					for (int i = 0; i < 3; i++)
						result->Offset[i] = (float)p.ReadDouble();
				}
				else if (p.LookAhead(L"CHANNELS"))
				{
					p.ReadToken();
					int count = p.ReadInt();
					for (int i = 0; i < count; i++)
					{
						auto channelName = p.ReadToken().Str;
						ChannelType ct;
						if (channelName == L"Xposition")
							ct = ChannelType::XPos;
						else if (channelName == L"Yposition")
							ct = ChannelType::YPos;
						else if (channelName == L"Zposition")
							ct = ChannelType::ZPos;
						else if (channelName == L"Xrotation")
							ct = ChannelType::XRot;
						else if (channelName == L"Yrotation")
							ct = ChannelType::YRot;
						else if (channelName == L"Zrotation")
							ct = ChannelType::ZRot;
						else if (channelName == L"Xscale")
							ct = ChannelType::XScale;
						else if (channelName == L"Yscale")
							ct = ChannelType::YScale;
						else if (channelName == L"Zscale")
							ct = ChannelType::ZScale;
						else
							throw TextFormatException(String(L"invalid channel type: ") + channelName);
						result->Channels.Add(ct);
					}
				}
				else if (p.LookAhead(L"JOINT"))
					result->SubJoints.Add(ParseNode(p));
				else
					throw TextFormatException(String(L"invalid Bvh field: ") + p.NextToken().Str);
			}
			p.Read(L"}");
			return result;
		}
		BvhFile BvhFile::FromFile(const CoreLib::String & fileName)
		{
			BvhFile result;
			Parser parser(File::ReadAllText(fileName));
			
			if (parser.LookAhead(L"HIERARCHY"))
			{
				parser.ReadToken();
				result.Hierarchy = ParseNode(parser);
			}

			if (parser.LookAhead(L"MOTION"))
			{
				parser.ReadToken();
				parser.Read(L"Frames");
				parser.Read(L":");
				int frameCount = parser.ReadInt();
				parser.Read(L"Frame"); parser.Read(L"Time"); parser.Read(L":");
				result.FrameDuration = (float)parser.ReadDouble();
				while (!parser.IsEnd())
				{
					result.FrameData.Add((float)parser.ReadDouble());
				}
			}
			return result;
		}
	}
}
