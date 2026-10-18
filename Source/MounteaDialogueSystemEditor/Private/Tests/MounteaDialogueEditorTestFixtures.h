// Copyright (C) 2026 Dominik (Pavlicek) Morse. All rights reserved.
//
// Developed for the Mountea Framework as a free tool. This solution is provided
// for use and sharing without charge. Redistribution is allowed under the following conditions:
//
// - You may use this solution in commercial products, provided the product is not
//   this solution itself (or unless significant modifications have been made to the solution).
// - You may not resell or redistribute the original, unmodified solution.
//
// For more information, visit: https://mountea.tools

#pragma once

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Archive/MounteaDialogueArchive.h"
#include "Archive/MounteaDialogueArchiveZip.h"
#include "Data/MounteaDialogueDataTable.h"
#include "Data/MounteaDialogueGraphDataTypes.h"
#include "Ed/EdGraph_MounteaDialogueGraph.h"
#include "Ed/EdNode_MounteaDialogueGraphNode.h"
#include "Edges/MounteaDialogueGraphEdge.h"
#include "GraphScheme/AssetGraphScheme_MounteaDialogueGraph.h"
#include "Graph/MounteaDialogueGraph.h"
#include "HAL/FileManager.h"
#include "Helpers/MounteaDialogueSystemImportExportHelpers.h"
#include "Interfaces/IPluginManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Nodes/MounteaDialogueGraphNode_DialogueNodeBase.h"
#include "UObject/Package.h"

// Shared by the editor tests: golden archive loading, project-file backups, and in-memory graphs (runtime graph
// plus, when needed, its editor EdGraph).

namespace MounteaArchiveTest
{
	inline bool LoadFixture(const TCHAR* Name, TArray<uint8>& OutBytes)
	{
		const TSharedPtr<IPlugin> plugin = IPluginManager::Get().FindPlugin(TEXT("MounteaDialogueSystem"));
		if (!plugin.IsValid())
			return false;
		const FString path = FPaths::Combine(plugin->GetBaseDir(), TEXT("Source/MounteaDialogueSystemEditor/Private/Tests/Fixtures"), Name);
		return FFileHelper::LoadFileToArray(OutBytes, *path);
	}

	inline bool ReadFixture(FAutomationTestBase& Test, const TCHAR* Name, MounteaArchive::FDialogueArchive& OutArchive)
	{
		TArray<uint8> bytes;
		if (!LoadFixture(Name, bytes))
		{
			Test.AddError(FString::Printf(TEXT("Fixture %s could not be loaded"), Name));
			return false;
		}
		TArray<FString> errors;
		if (!MounteaArchive::ReadArchiveFromBytes(bytes, OutArchive, errors))
		{
			for (const FString& error : errors)
				Test.AddError(error);
			return false;
		}
		return true;
	}

	inline bool LoadEntries(FAutomationTestBase& Test, const TCHAR* Name, MounteaArchive::FEntries& OutEntries)
	{
		TArray<uint8> bytes;
		TArray<FString> errors;
		if (!LoadFixture(Name, bytes) || !MounteaArchive::UnzipEntries(bytes, OutEntries, errors))
		{
			Test.AddError(FString::Printf(TEXT("Fixture %s could not be unzipped"), Name));
			return false;
		}
		return true;
	}

	inline bool HasError(const TArray<FString>& Errors, const TCHAR* Code)
	{
		return Errors.ContainsByPredicate([Code](const FString& Error) { return Error.StartsWith(Code); });
	}
}

namespace MounteaEditorTest
{
	// Restores a project file to what it was before the test (or deletes it if it did not exist).
	struct FScopedFileBackup
	{
		explicit FScopedFileBackup(const FString& InPath)
			: Path(InPath)
		{
			bExisted = FFileHelper::LoadFileToArray(Original, *Path);
		}

		~FScopedFileBackup()
		{
			if (bExisted)
				FFileHelper::SaveArrayToFile(Original, *Path);
			else
				IFileManager::Get().Delete(*Path);
		}

	private:
		FString Path;
		TArray<uint8> Original;
		bool bExisted = false;
	};

	// Scratch folder under Saved/, removed when the test ends.
	struct FScopedTempDir
	{
		explicit FScopedTempDir(const TCHAR* Name)
			: Path(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("MounteaEditorTests"), Name))
		{
			IFileManager::Get().MakeDirectory(*Path, true);
		}

		~FScopedTempDir()
		{
			IFileManager::Get().DeleteDirectory(*Path, false, true);
			IFileManager::Get().DeleteDirectory(*FPaths::GetPath(Path), false, false); // only if now empty
		}

		FString File(const TCHAR* Name) const { return FPaths::Combine(Path, Name); }

		FString Path;
	};

	// A transient runtime graph with a row table, wired like the importer does, plus an optional EdGraph.
	struct FEditorTestGraph
	{
		UMounteaDialogueGraph* Graph = nullptr;
		UMounteaDialogueDataTable* Table = nullptr;
		UEdGraph_MounteaDialogueGraph* EdGraph = nullptr;

		explicit FEditorTestGraph(const TCHAR* Name = nullptr)
		{
			Graph = NewObject<UMounteaDialogueGraph>(GetTransientPackage(), Name ? MakeUniqueObjectName(GetTransientPackage(), UMounteaDialogueGraph::StaticClass(), Name) : NAME_None);
			Table = NewObject<UMounteaDialogueDataTable>(GetTransientPackage());
			Table->RowStruct = FDialogueRow::StaticStruct();
		}

		UMounteaDialogueGraphNode* Start() const { return Graph->StartNode; }

		FName AddRow(const FName RowName, const TArray<FDialogueRowData>& Lines, const FName Participant = TEXT("NPC"))
		{
			FDialogueRow row;
			row.DialogueParticipantName = Participant;
			row.RowTitle = FText::FromName(RowName);
			row.RowData = Lines;
			Table->AddRow(RowName, row);
			return RowName;
		}

		template<typename TNode>
		TNode* AddNode(const FName RowName = NAME_None)
		{
			TNode* node = Graph->ConstructDialogueNode<TNode>();
			node->Graph = Graph;
			Graph->AllNodes.Add(node);
			if constexpr (TIsDerivedFrom<TNode, UMounteaDialogueGraphNode_DialogueNodeBase>::Value)
			{
				if (!RowName.IsNone())
				{
					node->SetDataTable(Table);
					node->SetRowName(RowName);
				}
			}
			return node;
		}

		UMounteaDialogueGraphEdge* Link(UMounteaDialogueGraphNode* Parent, UMounteaDialogueGraphNode* Child)
		{
			UMounteaDialogueGraphEdge* edge = NewObject<UMounteaDialogueGraphEdge>(Graph);
			edge->Graph = Graph;
			edge->StartNode = Parent;
			edge->EndNode = Child;
			Child->ExecutionOrder = Parent->ChildrenNodes.Num();
			Parent->ChildrenNodes.AddUnique(Child);
			Parent->Edges.Add(Child, edge);
			Child->ParentNodes.AddUnique(Parent);
			return edge;
		}

		// Creates the editor graph the way the asset editor does, with one EdNode per runtime node.
		UEdGraph_MounteaDialogueGraph* MakeEdGraph()
		{
			EdGraph = CastChecked<UEdGraph_MounteaDialogueGraph>(FBlueprintEditorUtils::CreateNewGraph(
				Graph, NAME_None, UEdGraph_MounteaDialogueGraph::StaticClass(), UAssetGraphScheme_MounteaDialogueGraph::StaticClass()));
			Graph->EdGraph = EdGraph;
			EdGraph->GetSchema()->CreateDefaultNodesForGraph(*EdGraph);
			for (UMounteaDialogueGraphNode* node : Graph->AllNodes)
				AddEdNode(node);
			return EdGraph;
		}

		UEdNode_MounteaDialogueGraphNode* AddEdNode(UMounteaDialogueGraphNode* Node)
		{
			UEdNode_MounteaDialogueGraphNode* edNode = EdGraph->CreateIntermediateNode<UEdNode_MounteaDialogueGraphNode>();
			edNode->SetMounteaDialogueGraphNode(Node);
			edNode->NodeGuid = Node->GetNodeGUID();
			edNode->AllocateDefaultPins();
			EdGraph->NodeMap.Add(Node, edNode);
			return edNode;
		}

		UEdNode_MounteaDialogueGraphNode* EdNodeOf(UMounteaDialogueGraphNode* Node) const
		{
			return EdGraph ? EdGraph->NodeMap.FindRef(Node) : nullptr;
		}
	};

	inline FDialogueRowData Line(const TCHAR* Text, const ERowExecutionMode Mode = ERowExecutionMode::EREM_Automatic,
		const ERowDurationMode DurationMode = ERowDurationMode::ERDM_Duration, const float Duration = 1.f, const float Override = 0.f)
	{
		return FDialogueRowData(FText::FromString(Text), nullptr, DurationMode, Duration, Override, Mode);
	}

	// Exports through the public entry point and reads the file back with the archive reader.
	inline bool ExportAndRead(FAutomationTestBase& Test, const UMounteaDialogueGraph* Graph, const FString& Path, MounteaArchive::FDialogueArchive& OutArchive)
	{
		if (!UMounteaDialogueSystemImportExportHelpers::ExportDialogueGraph(Graph, Path))
		{
			Test.AddError(TEXT("Export failed"));
			return false;
		}
		TArray<uint8> bytes;
		TArray<FString> errors;
		if (!FFileHelper::LoadFileToArray(bytes, *Path) || !MounteaArchive::ReadArchiveFromBytes(bytes, OutArchive, errors))
		{
			for (const FString& error : errors)
				Test.AddError(error);
			return false;
		}
		return true;
	}
}

#endif // WITH_DEV_AUTOMATION_TESTS
