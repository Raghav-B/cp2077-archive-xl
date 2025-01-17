#include "Module.hpp"

namespace
{
constexpr auto ModuleName = "QuestPhase";
}

std::string_view App::QuestPhaseModule::GetName()
{
    return ModuleName;
}

bool App::QuestPhaseModule::Load()
{
    if (!HookBefore<Raw::QuestLoader::ProcessPhaseResource>(&QuestPhaseModule::OnPhasePreload))
        throw std::runtime_error("Failed to hook [QuestLoader::ProcessPhaseResource].");

    // if (!HookBefore<Raw::QuestPhase::SetLoadedResource>(&QuestPhaseModule::OnPhaseReady))
    //     throw std::runtime_error("Failed to hook [QuestPhase::SetLoadedResource].");

    PreparePhases();

    return true;
}

void App::QuestPhaseModule::Reload()
{
    PreparePhases();
}

bool App::QuestPhaseModule::Unload()
{
    Unhook<Raw::QuestLoader::ProcessPhaseResource>();
    // Unhook<Raw::QuestPhase::SetLoadedResource>();

    return true;
}

void App::QuestPhaseModule::PreparePhases()
{
    s_phases.clear();

    auto depot = Red::ResourceDepot::Get();
    Core::Set<Red::ResourcePath> invalidPaths;

    for (auto& unit : m_units)
    {
        for (auto& phaseMod : unit.phases)
        {
            auto phasePath = Red::ResourcePath(phaseMod.phasePath.data());

            if (!depot->ResourceExists(phasePath))
            {
                if (!invalidPaths.contains(phasePath))
                {
                    LogWarning("|{}| Phase \"{}\" doesn't exist. Skipped.", ModuleName, phaseMod.phasePath);
                    invalidPaths.insert(phasePath);
                }
                continue;
            }

            auto parentPath = Red::ResourcePath(phaseMod.parentPath.data());

            if (!depot->ResourceExists(parentPath))
            {
                if (!invalidPaths.contains(parentPath))
                {
                    LogWarning("|{}| Phase \"{}\" doesn't exist. Skipped.", ModuleName, phaseMod.parentPath);
                    invalidPaths.insert(parentPath);
                }
                continue;
            }

            s_phases[parentPath].emplace_back(std::move(phaseMod));
        }

        unit.phases.clear();
    }
}

void App::QuestPhaseModule::OnPhasePreload(void* aLoader, Red::ResourcePath aPhasePath,
                                           Red::Handle<Red::questQuestPhaseResource>& aPhaseResource)
{
    PatchPhase(aPhaseResource);
}

// void App::QuestPhaseModule::OnPhaseReady(Red::questPhaseNodeDefinition* aPhaseNode,
//                                         Red::Handle<Red::questQuestPhaseResource>& aPhaseResource)
// {
//     PatchPhase(aPhaseResource);
// }

void App::QuestPhaseModule::PatchPhase(Red::Handle<Red::questQuestPhaseResource>& aPhaseResource)
{
    const auto& phaseMods = s_phases.find(aPhaseResource->path);

    if (phaseMods == s_phases.end())
        return;

    LogInfo("|{}| Patching phase \"{}\"...", ModuleName, phaseMods.value().begin()->parentPath);

    for (const auto& phaseMod : phaseMods.value())
    {
        if (PatchPhase(aPhaseResource, phaseMod))
        {
            LogInfo(R"(|{}| Merged phase "{}" from "{}".)", ModuleName, phaseMod.phasePath, phaseMod.mod);
        }
        else
        {
            LogWarning(R"(|{}| Can't merge phase "{}" from "{}".)", ModuleName, phaseMod.phasePath, phaseMod.mod);
        }
    }
}

bool App::QuestPhaseModule::PatchPhase(Red::Handle<Red::questQuestPhaseResource>& aPhaseResource,
                                       const App::QuestPhaseMod& aPhaseMod)
{
    auto& rootPhaseGraph = Red::Cast<Red::questGraphDefinition>(aPhaseResource->graph);
    if (!rootPhaseGraph)
        return false;

    auto [targetPhaseGraph, targetNode] = FindConnectionPoint(rootPhaseGraph, aPhaseMod.connection);
    if (!targetPhaseGraph || !targetNode)
        return false;

    auto modPhaseNode = CreatePhaseNode(targetPhaseGraph, aPhaseMod);
    if (!modPhaseNode)
        return false;

    AddConnection(targetNode, modPhaseNode);

    return true;
}

App::QuestPhaseModule::ConnectionPoint App::QuestPhaseModule::FindConnectionPoint(
    const Red::Handle<Red::questGraphDefinition>& aPhaseGraph, const Core::Vector<uint16_t>& aNodePath, uint32_t aStep)
{
    for (const auto& node : aPhaseGraph->nodes)
    {
        if (auto questNode = Red::Cast<Red::questNodeDefinition>(node))
        {
            if (questNode->id == aNodePath[aStep])
            {
                if (aStep == aNodePath.size() - 1)
                {
                    return {aPhaseGraph, questNode};
                }

                if (const auto& phaseNode = Red::Cast<Red::questPhaseNodeDefinition>(node))
                {
                    if (phaseNode->phaseGraph)
                    {
                        return FindConnectionPoint(phaseNode->phaseGraph, aNodePath, aStep + 1);
                    }
                }

                break;
            }
        }
    }

    return {};
}

Red::Handle<Red::questSocketDefinition> App::QuestPhaseModule::ResolveSocket(
    Red::Handle<Red::questNodeDefinition>& aNode, Red::questSocketType aSocketType, Red::CName aSocketName)
{
    for (const auto& graphSocket : aNode->sockets)
    {
        if (const auto& questSocket = Red::Cast<Red::questSocketDefinition>(graphSocket))
        {
            if (questSocket->type == aSocketType && questSocket->name == aSocketName)
            {
                return questSocket;
            }
        }
    }

    auto questSocket = Red::MakeHandle<Red::questSocketDefinition>();
    questSocket->type = aSocketType;
    questSocket->name = aSocketName;

    aNode->sockets.PushBack(questSocket);

    Raw::QuestSocketDefinition::OwnerNode::Ref(questSocket.instance) = aNode;

    return questSocket;
}

void App::QuestPhaseModule::AddConnection(Red::Handle<Red::questNodeDefinition>& aOut,
                                          Red::Handle<Red::questNodeDefinition>& aIn)
{
    auto inSocketName = aIn->GetType()->IsA(Red::GetClass<Red::questPhaseNodeDefinition>()) ? "In1" : "In";

    auto outSocket = ResolveSocket(aOut, Red::questSocketType::Output, "Out");
    auto inSocket = ResolveSocket(aIn, Red::questSocketType::Input, inSocketName);

    auto connection = Red::MakeHandle<Red::graphGraphConnectionDefinition>();
    connection->source = outSocket;
    connection->destination = inSocket;

    outSocket->connections.PushBack(connection);
    inSocket->connections.PushBack(connection);
}

Red::Handle<Red::questPhaseNodeDefinition> App::QuestPhaseModule::CreatePhaseNode(
    const Red::Handle<Red::questGraphDefinition>& aPhaseGraph, const QuestPhaseMod& aPhaseMod)
{
    auto phaseNodeId = GeneratePhaseNodeID(aPhaseMod.phasePath.data(), aPhaseMod.phasePath.size());

    for (const auto& node : aPhaseGraph->nodes)
    {
        if (auto questNode = Red::Cast<Red::questNodeDefinition>(node))
        {
            if (questNode->id == phaseNodeId)
            {
                return {};
            }
        }
    }

    auto phaseNode = Red::MakeHandle<Red::questPhaseNodeDefinition>();
    phaseNode->phaseResource = {aPhaseMod.phasePath.data()};
    phaseNode->id = phaseNodeId;

    aPhaseGraph->nodes.PushBack(phaseNode);

    return phaseNode;
}

uint16_t App::QuestPhaseModule::GeneratePhaseNodeID(const char* aData, uint32_t aLength)
{
    uint8_t x;
    uint16_t crc = 0xFFFF;

    while (aLength--)
    {
        x = crc >> 8 ^ *aData++;
        x ^= x>>4;
        crc = (crc << 8) ^ ((uint16_t)(x << 12)) ^ ((uint16_t)(x <<5)) ^ ((uint16_t)x);
    }

    return crc;
}
