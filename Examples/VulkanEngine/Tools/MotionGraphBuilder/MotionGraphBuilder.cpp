#include "CoreLib/Basic.h"
#include "CoreLib/LibIO.h"
#include "CoreLib/Basic.h"
#include "CoreLib/VectorMath.h"
#include "CoreLib/Parser.h"
#include "GameEngineCore/Skeleton.h"
#include "GameEngineCore/MotionGraph.h"
#include "MotionGraphBuilder.h"

namespace GameEngine
{
    using namespace CoreLib::IO;
    using namespace CoreLib::Text;

    namespace Tools
    {
        using Vec3List = List<VectorMath::Vec3>;
        using PoseMatrices = List<VectorMath::Matrix4>;

        List<Pose> GetPoseSequence(const Skeleton & skeleton, const SkeletalAnimation & anim)
        {
            List<Pose> result;
            for (int i = 0; i < anim.Channels.First().KeyFrames.Count(); i++)
            {
                Pose p;
                p.Transforms.SetSize(skeleton.Bones.Count());
                for (int j = 0; j < skeleton.Bones.Count(); j++)
                {
                    p.Transforms[j] = skeleton.Bones[j].BindPose;
                }
                for (auto & channel : anim.Channels)
                {
                    int boneId = -1;
                    if (skeleton.BoneMapping.TryGetValue(channel.BoneName, boneId))
                    {
                        p.Transforms[boneId] = channel.KeyFrames[i].Transform;
                    }
                }
                
                result.Add(_Move(p));
            }
            return result;
        }

        List<PoseMatrices> ConvertPoseToMatrices(const Skeleton & skeleton, const List<Pose> & poses)
        {
            List<PoseMatrices> result;

            for (int i = 0; i < poses.Count(); i++)
            {
                PoseMatrices matrices;
                poses[i].GetMatrices(&skeleton, matrices);
                result.Add(_Move(matrices));
            }
            return result;
        }
        
        void ConvertPoseToLocationVelocity(const Skeleton & skeleton, const List<Pose> & poses, List<Vec3List> & boneLocations, List<Vec3List> & boneVelocity)
        {
            auto poseMatrices = ConvertPoseToMatrices(skeleton, poses);
            int numPose = poseMatrices.Count();
            int numBone = poseMatrices[0].Count();
            boneLocations.SetSize(numPose);
            boneVelocity.SetSize(numPose);

            for (int i = 0; i < numPose; i++)
            {
                boneLocations[i].SetSize(numBone);
                for (int j = 0; j < numBone; j++)
                {
                    boneLocations[i][j].x = poseMatrices[i][j].values[12];
                    boneLocations[i][j].y = poseMatrices[i][j].values[13];
                    boneLocations[i][j].z = poseMatrices[i][j].values[14];
                }
            }

            for (int i = 0; i < numPose; i++)
            {
                boneVelocity[i].SetSize(numBone);
                for (int j = 0; j < numBone; j++)
                {
                    if (i > 0)
                    {
                        boneVelocity[i][j].x = boneLocations[i][j].x - boneLocations[i - 1][j].x;
                        boneVelocity[i][j].y = boneLocations[i][j].y - boneLocations[i - 1][j].y;
                        boneVelocity[i][j].z = boneLocations[i][j].z - boneLocations[i - 1][j].z;
                    }
                    else
                    {
                        boneVelocity[i][j].x = boneLocations[i + 1][j].x - boneLocations[i][j].x;
                        boneVelocity[i][j].y = boneLocations[i + 1][j].y - boneLocations[i][j].y;
                        boneVelocity[i][j].z = boneLocations[i + 1][j].z - boneLocations[i][j].z;
                    }
                }
            }

        }

        float CalculateVec3Distance(VectorMath::Vec3 v1, VectorMath::Vec3 v2)
        {
            return sqrt(pow((v2.x - v1.x), 2) +
                        pow((v2.y - v1.y), 2) +
                        pow((v2.z - v1.z), 2));
        }

        float CalculateStateDistance(const MGState & s1, const MGState & s2)
        {
            const Vec3List & lastLocations = s1.Positions;
            const Vec3List & lastVelocity = s1.Velocities;
            const Vec3List & currLocations = s2.Positions;
            const Vec3List & currVelocity = s2.Velocities;

            int numBone = lastLocations.Count();
            float distance = 0.0f;
            float locationWeight = 0.5f;
            float velocityWeight = 0.5f;

            for (int i = 0; i < numBone; i++)
            {
                distance = Math::Max(distance, 
                    locationWeight * CalculateVec3Distance(lastLocations[i], currLocations[i]) +
                    velocityWeight * CalculateVec3Distance(lastVelocity[i], currVelocity[i]));
            }
            return distance;
        }

        void MergeDuplicateStates(MotionGraph & graph, float distanceThresholdFactor)
        {
            CoreLib::Dictionary<int, int> replaceStates;

            float maxDist = 0.0f;
            for (int i = 1; i < graph.States.Count(); i++)
            {
                maxDist = Math::Max(maxDist, CalculateStateDistance(graph.States[i - 1], graph.States[i]));
            }
            float distanceThreshold = maxDist * distanceThresholdFactor;

            for (int i = 0; i < graph.States.Count(); i++)
            {
                auto & state = graph.States[i];
                for (int j = 0; j < i; j++)
                {
                    if (!replaceStates.ContainsKey(j))
                    {
                        if (CalculateStateDistance(state, graph.States[j]) < distanceThreshold)
                        {
                            replaceStates[i] = j;
                            for (auto child : graph.States[i].ChildrenIds)
                                graph.States[j].ChildrenIds.Add(child);
                            break;
                        }
                    }
                }
            }

            // replace child ids
            auto replaceChildIds = [graph](const Dictionary<int, int> & replaceMapping)
            {
                for (auto & state : graph.States)
                {
                    EnumerableHashSet<int> newChildIds;
                    for (auto & childId : state.ChildrenIds)
                    {
                        int newId = childId;
                        replaceMapping.TryGetValue(childId, newId);
                        newChildIds.Add(newId);
                    }
                    state.ChildrenIds = _Move(newChildIds);
                }
            };

            replaceChildIds(replaceStates);

            // remove and compact state list
            List<MGState> newStates;
            Dictionary<int, int> newStateIdMapping;
            for (int i = 0; i < graph.States.Count(); i++)
            {
                if (!replaceStates.ContainsKey(i))
                {
                    newStateIdMapping[i] = newStates.Count();
                    newStates.Add(graph.States[i]);
                }
            }

            replaceChildIds(newStateIdMapping);
        }

        MotionGraph BuildMotionGraph(const CoreLib::String & filename)
        {
            MotionGraph graph;
            auto lines = Split(File::ReadAllText(filename), L'\n');
            
            String parentPath = Path::GetDirectoryName(filename);

            Skeleton skeleton;
            skeleton.LoadFromFile(Path::Combine(parentPath, lines[0]));
            
            String dir = Path::Combine(parentPath, lines[1]);
            auto anims = From(lines).Skip(2).Select([&](String name)
            {
                SkeletalAnimation anim; 
                anim.LoadFromFile(Path::Combine(dir, name)); 
                return anim; 
            }).ToList();

            if (anims.Count() > 0)      // by default, the graph will share same duration
            {
                graph.Duration = anims[0].Duration;
                graph.Speed = anims[0].Speed;
            }

            for (auto & anim : anims)
            {
                
                auto poses = GetPoseSequence(skeleton, anim);
                
                List<Vec3List> boneLocations;
                List<Vec3List> boneVelocities;
                ConvertPoseToLocationVelocity(skeleton, poses, boneLocations, boneVelocities);

                for (int i = 0; i < poses.Count(); i++)
                {
                    MGState state;
                    state.Positions = boneLocations[i];
                    state.Velocities = boneVelocities[i];
                    state.Pose = poses[i];
                    if (i != poses.Count() - 1)
                        state.ChildrenIds.Add(graph.States.Count() + 1);
                    graph.States.Add(_Move(state));
                }
            }
            MergeDuplicateStates(graph, 1.5f);

            return graph;
        }
    }
}

using namespace GameEngine;

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        printf("Usage: MotionGraphBuilder -dataset <filename>\n");
        return -1;
    }
    if (String(argv[1]) == "-dataset")
    {
        MotionGraph graph = Tools::BuildMotionGraph(argv[2]);
        graph.SaveToFile(Path::ReplaceExt(argv[2], L"mog"));
    }
    else
        printf("Invalid arguments.\n");
	return 0;
}


