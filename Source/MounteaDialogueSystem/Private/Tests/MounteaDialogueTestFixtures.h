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

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR // editor-only: needs an editor world, kept out of game builds

#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "Subsystem/MounteaDialogueViewportHUDSubsystem.h"
#include "Subsystem/MounteaDialogueLocalPlayerSubsystem.h"
#include "Subsystem/MounteaDialogueWorldSubsystem.h"
#include "Components/MounteaDialogueManager.h"
#include "Components/MounteaDialogueParticipant.h"
#include "Components/MounteaDialogueSession.h"
#include "Data/MounteaDialogueContext.h"
#include "Data/MounteaDialogueDataTable.h"
#include "Data/MounteaDialogueGraphDataTypes.h"
#include "Edges/MounteaDialogueGraphEdge.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "Graph/MounteaDialogueGraph.h"
#include "Misc/DataValidation.h"
#include "Nodes/MounteaDialogueGraphNode_DialogueNodeBase.h"
#include "Nodes/MounteaDialogueGraphNode_ReturnToNode.h"
#include "Settings/MounteaDialogueSystemSettings.h"
#include "UObject/Package.h"

// Shared across the Tests/ folder. A real UWorld (FTestWorldWrapper) plus a ULocalPlayer registered via
// PlayerAdded() is the minimum needed for APlayerController::GetLocalPlayer()->GetSubsystem<>() to
// resolve to a live subsystem. Widget creation / viewport attachment needing a real viewport is
// deliberately not exercised by tests built on this fixture.
namespace MounteaDialogueTest
{
	// Owns the test world, a PlayerController, and a registered ULocalPlayer for the duration of a test.
	struct FPlayerControllerFixture
	{
		FTestWorldWrapper WorldWrapper;
		APlayerController* PlayerController = nullptr;
		ULocalPlayer* LocalPlayer = nullptr;
		UMounteaDialogueViewportHUDSubsystem* Subsystem = nullptr;

		UWorld* GetWorld() const { return WorldWrapper.GetTestWorld(); }

		bool Setup(FAutomationTestBase& Test)
		{
			// Projects using CommonUI without a CommonGameViewportClient log this error on any world
			// bring-up. It is unrelated to what is under test, but the automation framework treats every
			// logged error as a failure. Occurrence count 0 = at least once (this bring-up always logs it).
			Test.AddExpectedError(TEXT("Using CommonUI without a CommonGameViewportClient"), EAutomationExpectedErrorFlags::Contains, 0);

			if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
			{
				Test.AddError(TEXT("Failed to create test world"));
				return false;
			}

			UWorld* world = WorldWrapper.GetTestWorld();
			if (!world)
			{
				Test.AddError(TEXT("Test world is null"));
				return false;
			}
			WorldWrapper.BeginPlayInTestWorld();

			PlayerController = world->SpawnActor<APlayerController>();
			if (!PlayerController)
			{
				Test.AddError(TEXT("Failed to spawn PlayerController"));
				return false;
			}

			LocalPlayer = NewObject<ULocalPlayer>(GEngine);
			if (!LocalPlayer)
			{
				Test.AddError(TEXT("Failed to create LocalPlayer"));
				return false;
			}

			// Initializes the LocalPlayer's subsystem collection, which is what makes
			// GetSubsystem<UMounteaDialogueViewportHUDSubsystem>() non-null.
			LocalPlayer->PlayerAdded(nullptr, FPlatformUserId::CreateFromInternalId(0));
			PlayerController->Player = LocalPlayer;

			Subsystem = LocalPlayer->GetSubsystem<UMounteaDialogueViewportHUDSubsystem>();
			return true;
		}

		void Teardown(FAutomationTestBase& Test)
		{
			if (LocalPlayer)
			{
				LocalPlayer->PlayerRemoved();
				LocalPlayer = nullptr;
			}
			WorldWrapper.ForwardErrorMessages(&Test);
			WorldWrapper.DestroyTestWorld(true);
		}
	};

	// An in-memory dialogue graph plus its row DataTable, both on the transient package. Wires nodes the same way
	// the importer does (PopulateEdges): children, parents, one edge per link, sibling ExecutionOrder.
	struct FTestGraph
	{
		UMounteaDialogueGraph* Graph = nullptr;
		UMounteaDialogueDataTable* Table = nullptr;

		FTestGraph()
		{
			// PostInitProperties creates the Start node (editor builds).
			Graph = NewObject<UMounteaDialogueGraph>(GetTransientPackage());
			Table = NewObject<UMounteaDialogueDataTable>(GetTransientPackage());
			Table->RowStruct = FDialogueRow::StaticStruct();
		}

		UMounteaDialogueGraphNode* Start() const { return Graph->StartNode; }

		// Adds a row whose lines each last Seconds (Override mode, so no sound or settings are involved).
		FName AddRow(const FName RowName, const TArray<ERowExecutionMode>& Lines, const float Seconds = 0.1f, const FName Participant = TEXT("NPC"))
		{
			FDialogueRow row;
			row.DialogueParticipantName = Participant;
			row.RowTitle = FText::FromName(RowName);
			for (int32 i = 0; i < Lines.Num(); ++i)
			{
				row.RowData.Add(FDialogueRowData(FText::FromString(FString::Printf(TEXT("%s line %d"), *RowName.ToString(), i)),
					nullptr, ERowDurationMode::EDRM_Override, 0.f, Seconds, Lines[i]));
			}
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

		// Speech node with its own row, named after the node.
		template<typename TNode>
		TNode* AddSpeech(const TCHAR* Name, const TArray<ERowExecutionMode>& Lines = { ERowExecutionMode::EREM_Automatic })
		{
			return AddNode<TNode>(AddRow(FName(Name), Lines));
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
	};

	// Sets a (usually protected) UPROPERTY by name, so tests can configure decorators and conditions the way the
	// details panel does, without friend declarations on every class. Fails the test on a typo.
	template<typename TValue>
	static bool SetProperty(FAutomationTestBase& Test, UObject* Object, const FName Name, const TValue& Value)
	{
		FProperty* property = Object ? FindFProperty<FProperty>(Object->GetClass(), Name) : nullptr;
		if (!property)
		{
			Test.AddError(FString::Printf(TEXT("No property '%s' on %s"), *Name.ToString(), *GetNameSafe(Object)));
			return false;
		}
		if constexpr (std::is_same_v<TValue, bool>)
			CastFieldChecked<FBoolProperty>(property)->SetPropertyValue_InContainer(Object, Value);
		else
			*property->ContainerPtrToValuePtr<TValue>(Object) = Value;
		return true;
	}

	// Reference to a (usually protected) UPROPERTY by name, e.g. a graph's GraphDecorators. Null on a typo.
	template<typename TValue>
	static TValue* PropertyRef(UObject* Object, const FName Name)
	{
		FProperty* property = Object ? FindFProperty<FProperty>(Object->GetClass(), Name) : nullptr;
		return property ? property->ContainerPtrToValuePtr<TValue>(Object) : nullptr;
	}

	// Instances a decorator inside its owner (node or graph), as the details panel does, and attaches it.
	template<typename TDecorator>
	static TDecorator* AddDecorator(UObject* Owner, TArray<FMounteaDialogueDecorator>& Decorators)
	{
		TDecorator* decorator = NewObject<TDecorator>(Owner);
		FMounteaDialogueDecorator entry;
		entry.DecoratorType = decorator;
		Decorators.Add(entry);
		return decorator;
	}

	// All validation messages (errors and warnings) in one string, for "mentions X" checks.
	static FString JoinIssues(const FDataValidationContext& Context)
	{
		FString result;
		for (const FDataValidationContext::FIssue& issue : Context.GetIssues())
			result += issue.Message.ToString() + TEXT("\n");
		return result;
	}

	// A Game world that can run a whole dialogue on the server: GameState with a UMounteaDialogueSession,
	// a player pawn holding the manager and the player participant, and an NPC holding the graph.
	// Standalone net mode, so every actor has authority and Client RPCs run locally.
	struct FDialogueWorldFixture
	{
		FTestWorldWrapper WorldWrapper;
		AGameStateBase* GameState = nullptr;
		UMounteaDialogueSession* Session = nullptr;
		UMounteaDialogueWorldSubsystem* Subsystem = nullptr;
		APawn* Player = nullptr;
		UMounteaDialogueManager* Manager = nullptr;
		UMounteaDialogueParticipant* PlayerParticipant = nullptr;
		AActor* Npc = nullptr;
		UMounteaDialogueParticipant* NpcParticipant = nullptr;

		UWorld* GetWorld() const { return WorldWrapper.GetTestWorld(); }

		template<typename TComponent>
		static TComponent* AddComponent(AActor* Owner)
		{
			TComponent* component = NewObject<TComponent>(Owner);
			Owner->AddInstanceComponent(component);
			component->RegisterComponent();
			return component;
		}

		// bWithSession=false leaves the GameState without a session, for the "missing session" paths.
		bool Setup(FAutomationTestBase& Test, UMounteaDialogueGraph* NpcGraph, const bool bWithSession = true)
		{
			if (!WorldWrapper.CreateTestWorld(EWorldType::Game) || !GetWorld())
			{
				Test.AddError(TEXT("Failed to create test world"));
				return false;
			}
			WorldWrapper.BeginPlayInTestWorld();
			UWorld* world = GetWorld();

			// BeginPlayInTestWorld creates a GameMode, which spawns the GameState.
			GameState = world->GetGameState();
			if (!GameState)
			{
				GameState = world->SpawnActor<AGameStateBase>();
				world->SetGameState(GameState);
			}
			if (!GameState)
			{
				Test.AddError(TEXT("Failed to find or spawn a GameState"));
				return false;
			}
			if (bWithSession)
				Session = AddComponent<UMounteaDialogueSession>(GameState);

			Subsystem = world->GetSubsystem<UMounteaDialogueWorldSubsystem>();

			Player = world->SpawnActor<APawn>();
			Npc = world->SpawnActor<AActor>();
			if (!Player || !Npc || !Subsystem)
			{
				Test.AddError(TEXT("Failed to spawn participants or find the world subsystem"));
				return false;
			}

			Manager = AddComponent<UMounteaDialogueManager>(Player);
			PlayerParticipant = AddComponent<UMounteaDialogueParticipant>(Player);
			NpcParticipant = AddComponent<UMounteaDialogueParticipant>(Npc);
			if (NpcGraph)
				IMounteaDialogueParticipantInterface::Execute_SetDialogueGraph(NpcParticipant, NpcGraph);
			return true;
		}

		// Starts the dialogue the way gameplay code does: player manager, NPC as main participant.
		void StartDialogue(UMounteaDialogueManager* InManager = nullptr, AActor* MainParticipant = nullptr)
		{
			FDialogueParticipants participants;
			participants.MainParticipant = MainParticipant ? MainParticipant : Npc;
			IMounteaDialogueManagerInterface::Execute_RequestStartDialogue(InManager ? InManager : Manager, Player, participants);
		}

		// Several small ticks: one large tick does not fire timers set during the same frame, and timers fire
		// one tick late (the first tick after SetTimer does not advance it). Allow ~2 extra ticks per timer.
		void Tick(const int32 Count, const float DeltaSeconds = 0.05f)
		{
			for (int32 i = 0; i < Count; ++i)
				WorldWrapper.TickTestWorld(DeltaSeconds);
		}

		EDialogueManagerState GetState() const { return IMounteaDialogueManagerInterface::Execute_GetManagerState(Manager); }
		UMounteaDialogueContext* GetContext() const { return IMounteaDialogueManagerInterface::Execute_GetDialogueContext(Manager); }
		const FMounteaDialogueContextPayload& GetPayload() const { return Session->GetContextPayload(); }

		UMounteaDialogueGraphNode* GetActiveNode() const
		{
			const UMounteaDialogueContext* context = GetContext();
			return IsValid(context) ? context->ActiveNode.Get() : nullptr;
		}

		void Teardown(FAutomationTestBase& Test)
		{
			WorldWrapper.ForwardErrorMessages(&Test);
			WorldWrapper.DestroyTestWorld(true);
		}
	};
}

// Failure paths log through LOG_ERROR, but only when the project's Mountea logging settings allow it, and keyed
// messages are throttled. Expected-error declarations cannot express "maybe", so tests that drive failures mute
// the plugin's logging for their duration instead; they assert on state, never on log output.
struct FScopedDialogueLogMute
{
	FScopedDialogueLogMute();
	~FScopedDialogueLogMute();

private:
	uint8 SavedVerbosity = 0;
};

// Grants the Tests/ folder access to the HUD subsystems' private wrapper state (see the friend declarations),
// so a test can seed "a wrapper left over from a previous world" without driving real widget creation.
// Same pattern as FMounteaInteractionTestAccess in MounteaInteractionSystem.
struct FMounteaDialogueTestAccess
{
	static void SeedViewportWidget(UMounteaDialogueViewportHUDSubsystem* Subsystem, UUserWidget* Widget, UWorld* WrapperWorld)
	{
		Subsystem->ViewportWidget = Widget;
		Subsystem->ViewportWidgetWorld = WrapperWorld;
	}

	static void SeedViewportWidget(UMounteaDialogueLocalPlayerSubsystem* Subsystem, UUserWidget* Widget, UWorld* WrapperWorld)
	{
		Subsystem->ViewportWidget = Widget;
		Subsystem->ViewportWidgetWorld = WrapperWorld;
	}

	static void OnDelayDurationExpired(UMounteaDialogueGraphNode_ReturnToNode* Node, const TScriptInterface<IMounteaDialogueManagerInterface>& Manager)
	{
		Node->OnDelayDurationExpired(Manager);
	}

	static FTimerHandle& TimerHandle(UMounteaDialogueGraphNode_ReturnToNode* Node)
	{
		return Node->TimerHandle_Delay;
	}

	static bool IsLockActive(const UMounteaDialogueWorldSubsystem* Subsystem)
	{
		return Subsystem->bDialogueLockActive;
	}

	static UMounteaDialogueManager* GetLockOwner(const UMounteaDialogueWorldSubsystem* Subsystem)
	{
		return Subsystem->ActiveManager.Get();
	}

	static bool TryAcquireDialogueLock(UMounteaDialogueWorldSubsystem* Subsystem, UMounteaDialogueManager* Manager)
	{
		return Subsystem->TryAcquireDialogueLock(Manager);
	}

	static uint8& LogVerbosity(UMounteaDialogueSystemSettings* Settings)
	{
		return Settings->LogVerbosity;
	}

	static TMap<FUIRowID, FMounteaSubtitlesSettings>& SubtitlesOverrides(UMounteaDialogueSystemSettings* Settings)
	{
		return Settings->SubtitlesSettingsOverrides;
	}

	static bool ResolveParticipants(const UMounteaDialogueWorldSubsystem* Subsystem, UMounteaDialogueManager* Manager,
		const FDialogueStartRequest& Request, TArray<TScriptInterface<IMounteaDialogueParticipantInterface>>& OutAll, TArray<FText>& OutErrors)
	{
		TScriptInterface<IMounteaDialogueParticipantInterface> player;
		TScriptInterface<IMounteaDialogueParticipantInterface> main;
		return Subsystem->ResolveParticipants(Manager, Request, player, main, OutAll, OutErrors);
	}
};

inline FScopedDialogueLogMute::FScopedDialogueLogMute()
{
	uint8& verbosity = FMounteaDialogueTestAccess::LogVerbosity(GetMutableDefault<UMounteaDialogueSystemSettings>());
	SavedVerbosity = verbosity;
	verbosity = 0;
}

inline FScopedDialogueLogMute::~FScopedDialogueLogMute()
{
	FMounteaDialogueTestAccess::LogVerbosity(GetMutableDefault<UMounteaDialogueSystemSettings>()) = SavedVerbosity;
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
