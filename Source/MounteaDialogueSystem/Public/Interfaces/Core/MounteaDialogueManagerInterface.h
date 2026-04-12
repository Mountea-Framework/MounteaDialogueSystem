// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "Data/MounteaDialogueGraphDataTypes.h"
#include "Data/MounteaDialogueUITypes.h"
#include "UObject/Interface.h"
#include "MounteaDialogueManagerInterface.generated.h"

class IMounteaDialogueParticipantInterface;

/**
 * A helper structure that provides easy way to move Participants around.
 */
USTRUCT(BlueprintType)
struct FDialogueParticipants
{
	GENERATED_BODY()

public:

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Mountea|Dialogue|Manager")
	TObjectPtr<AActor> MainParticipant = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Mountea|Dialogue|Manager")
	TArray<TObjectPtr<UObject>> OtherParticipants;
};

UINTERFACE(MinimalAPI, BlueprintType, Blueprintable)
class UMounteaDialogueManagerInterface : public UInterface
{
	GENERATED_BODY()
};

class UMounteaDialogueContext;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueEvent, UMounteaDialogueContext*, Context);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FDialogueStartRequested, const TScriptInterface<IMounteaDialogueManagerInterface>&, CallingManager, AActor*, DialogueInitiator, const FDialogueParticipants&, InitialParticipants);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDialogueStartRequestedResult, const bool, Result, const FString&, ResultMessage);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueContextUpdated, UMounteaDialogueContext*, Context);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDialogueUserInterfaceChanged, TSubclassOf<UUserWidget>, DialogueWidgetClass, UUserWidget*, DialogueWidget);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueNodeEvent, UMounteaDialogueContext*, Context);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueRowEvent, UMounteaDialogueContext*, Context);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueFailed, const FString&, ErrorMessage);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueManagerStateChanged, const EDialogueManagerState&, NewState);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDialogueWidgetCommand, const TScriptInterface<IMounteaDialogueManagerInterface>&, DialogueManager, const FString&, WidgetCommand);

/**
 * Fired on the owning client when a UI signal arrives via Client_DispatchUISignal.
 * Bound by UMounteaDialogueParticipantUserInterfaceComponent instances.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueUISignalEvent, const FMounteaDialogueUISignal&, Signal);

/**
 * Mountea Dialogue Manager Interface.
 * 
 * Should be attached directly to Player State.
 * Provides options to start and stop dialogue as well as ability to select dialogue options.
 */
class MOUNTEADIALOGUESYSTEM_API IMounteaDialogueManagerInterface
{
	GENERATED_BODY()

public:

	// --- Actor functions ------------------------------

	/**
	 * Returns the owning actor for this Dialogue Manager Component.
	 *
	 * @return The owning actor for this Dialogue Manager Component.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Manager")
	AActor* GetOwningActor() const;
	virtual AActor* GetOwningActor_Implementation() const = 0;

	/**
	 * Retrieves current Dialogue Manager State.
	 * State defines whether Manager can start/close dialogue or not.
	 * 
	 * @return ManagerState	Manager state value
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Manager")
	EDialogueManagerState GetManagerState() const;
	virtual EDialogueManagerState GetManagerState_Implementation() const = 0;
	
	/**
	 * Sets new Dialogue Manager State.
	 * 
	 * @param NewState	Manager State to be set as Manager State
	 */
	virtual void SetManagerState(const EDialogueManagerState NewState) = 0;

	/**
	 * Retrieves current Default Dialogue Manager State.
	 * Default Dialogue Manager State sets Dialogue Manager state upon BeginPlay and is used as fallback once Dialogue ends.
	 * 
	 * @return ManagerState	Default Manager state value
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Manager")
	EDialogueManagerState GetDefaultManagerState() const;
	virtual EDialogueManagerState GetDefaultManagerState_Implementation() const = 0;

	/**
	 * Sets new Default Dialogue Manager State.
	 * 
	 * @param NewState	Manager State to be set as Default Manager State
	 */
	virtual void SetDefaultManagerState(const EDialogueManagerState NewState) = 0;
	
	virtual EDialogueManagerType GetDialogueManagerType() const = 0;

	// --- Context functions ------------------------------
	
	/**
	 * Retrieves current Dialogue Context.
	 * 
	 * ❗ Could be null
	 * @return DialogueContext	Dialogue Context is transient data holder for current dialogue instance.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Manager")
	UMounteaDialogueContext* GetDialogueContext() const;
	virtual UMounteaDialogueContext* GetDialogueContext_Implementation() const = 0;

	/**
	 * Sets new Dialogue Context.
	 * 
	 * ❔ Null value clears saved data
	 * @param NewContext	Dialogue Context to be set as Dialogue Context
	 */
	virtual void SetDialogueContext(UMounteaDialogueContext* NewContext) = 0;

	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Manager")
	void UpdateDialogueContext(UMounteaDialogueContext* NewContext);
	virtual void UpdateDialogueContext_Implementation(UMounteaDialogueContext* NewContext) = 0;

	// --- Dialogue Lifecycle functions ------------------------------

	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Manager")
	bool CanStartDialogue() const;
	virtual bool CanStartDialogue_Implementation() const = 0;

	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Manager")
	void RequestStartDialogue(AActor* DialogueInitiator, const FDialogueParticipants& InitialParticipants);
	virtual void RequestStartDialogue_Implementation(AActor* DialogueInitiator, const FDialogueParticipants& InitialParticipants) = 0;

	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Manager")
	void StartDialogue();
	virtual void StartDialogue_Implementation() = 0;

	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Manager")
	void RequestCloseDialogue();
	virtual void RequestCloseDialogue_Implementation() = 0;

	/**
	 * Closes the Dialogue if is active.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Manager", meta=(Keywords="exit,close"))
	void CloseDialogue();
	virtual void CloseDialogue_Implementation() = 0;

	// --- World UI functions (deprecated — use UMounteaDialogueParticipantUserInterfaceComponent) ---

	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Manager",
		meta=(DeprecatedFunction),
		meta=(DeprecationMessage="Use IMounteaDialogueParticipantUIInterface instead."))
	void UpdateWorldDialogueUI(const FString& Command);
	virtual void UpdateWorldDialogueUI_Implementation(const FString& Command) = 0;

	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Manager",
		meta=(DeprecatedFunction),
		meta=(DeprecationMessage="Use IMounteaDialogueParticipantUIInterface instead."))
	bool AddDialogueUIObject(UObject* NewDialogueObject);
	virtual bool AddDialogueUIObject_Implementation(UObject* NewDialogueObject) = 0;

	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Manager",
		meta=(DeprecatedFunction),
		meta=(DeprecationMessage="Use IMounteaDialogueParticipantUIInterface instead."))
	bool AddDialogueUIObjects(const TArray<UObject* >& NewDialogueObjects);
	virtual bool AddDialogueUIObjects_Implementation(const TArray<UObject* >& NewDialogueObjects) = 0;

	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Manager",
		meta=(DeprecatedFunction),
		meta=(DeprecationMessage="Use IMounteaDialogueParticipantUIInterface instead."))
	bool RemoveDialogueUIObject(UObject* DialogueObjectToRemove);
	virtual bool RemoveDialogueUIObject_Implementation(UObject* DialogueObjectToRemove) = 0;

	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Manager",
		meta=(DeprecatedFunction),
		meta=(DeprecationMessage="Use IMounteaDialogueParticipantUIInterface instead."))
	bool RemoveDialogueUIObjects(const TArray<UObject* >& DialogueObjectsToRemove);
	virtual bool RemoveDialogueUIObjects_Implementation(const TArray<UObject* >& DialogueObjectsToRemove) = 0;

	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Manager",
		meta=(DeprecatedFunction),
		meta=(DeprecationMessage="Use IMounteaDialogueParticipantUIInterface instead."))
	void SetDialogueUIObjects(const TArray<UObject* >& NewDialogueObjects);
	virtual void SetDialogueUIObjects_Implementation(const TArray<UObject* >& NewDialogueObjects) = 0;

	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Manager",
		meta=(DeprecatedFunction),
		meta=(DeprecationMessage="Use IMounteaDialogueParticipantUIInterface instead."))
	void ResetDialogueUIObjects();
	virtual void ResetDialogueUIObjects_Implementation() = 0;

	// --- Screen UI functions (deprecated — use UMounteaDialogueParticipantUserInterfaceComponent) ---

	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Manager",
		meta=(DeprecatedFunction),
		meta=(DeprecationMessage="Use IMounteaDialogueParticipantUIInterface instead."))
	bool CreateDialogueUI(FString& Message);
	virtual bool CreateDialogueUI_Implementation(FString& Message) = 0;

	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Manager",
		meta=(DeprecatedFunction),
		meta=(DeprecationMessage="Use IMounteaDialogueParticipantUIInterface instead."))
	bool UpdateDialogueUI(FString& Message, const FString& Command);
	virtual bool UpdateDialogueUI_Implementation(FString& Message, const FString& Command) = 0;

	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Manager",
		meta=(DeprecatedFunction),
		meta=(DeprecationMessage="Use IMounteaDialogueParticipantUIInterface instead."))
	bool CloseDialogueUI();
	virtual bool CloseDialogueUI_Implementation() = 0;

	// --- General UI functions (deprecated — use UMounteaDialogueParticipantUserInterfaceComponent) ---

	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Manager",
		meta=(DeprecatedFunction),
		meta=(DeprecationMessage="Use IMounteaDialogueParticipantUIInterface instead."))
	void ExecuteWidgetCommand(const FString& Command);
	virtual void ExecuteWidgetCommand_Implementation(const FString& Command) = 0;

	UE_DEPRECATED(5.5, "Use UMounteaDialogueConfiguration::DefaultDialogueWidgetClass instead.")
	virtual TSubclassOf<UUserWidget> GetDialogueWidgetClass() const = 0;

	UE_DEPRECATED(5.5, "Use UMounteaDialogueConfiguration::DefaultDialogueWidgetClass instead.")
	virtual void SetDialogueWidgetClass(TSubclassOf<UUserWidget> NewWidgetClass) = 0;

	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Manager",
		meta=(DeprecatedFunction),
		meta=(DeprecationMessage="Use IMounteaDialogueParticipantUIInterface instead."))
	void SetDialogueWidget(UUserWidget* DialogueUIPtr);
	virtual void SetDialogueWidget_Implementation(UUserWidget* DialogueUIPtr) = 0;

	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Manager",
		meta=(DeprecatedFunction),
		meta=(DeprecationMessage="Use IMounteaDialogueParticipantUIInterface instead."))
	UUserWidget* GetDialogueWidget() const;
	virtual UUserWidget* GetDialogueWidget_Implementation() const = 0;

	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Manager",
		meta=(DeprecatedFunction),
		meta=(DeprecationMessage="Use UMounteaDialogueConfiguration::DefaultDialogueWidgetZOrder instead."))
	int32 GetDialogueWidgetZOrder() const;
	virtual int32 GetDialogueWidgetZOrder_Implementation() const = 0;

	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Manager",
		meta=(DeprecatedFunction),
		meta=(DeprecationMessage="Use UMounteaDialogueConfiguration::DefaultDialogueWidgetZOrder instead."))
	void SetDialogueWidgetZOrder(const int32 NewZOrder);
	virtual void SetDialogueWidgetZOrder_Implementation(const int32 NewZOrder) = 0;

	// --- Dialogue Loop functions ------------------------------

	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Manager")
	void CleanupDialogue();
	virtual void CleanupDialogue_Implementation() = 0;

	// --- Node Loop functions ------------------------------

	/**
	 * Prepares the node for execution.
	 * Asks Active Node to 'PreProcessNode' and then to 'ProcessNode'.
	 * In this preparation stage, Nodes are asked to process all Decorators.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Manager")
	void PrepareNode();
	virtual void PrepareNode_Implementation() = 0;

	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Manager")
	void NodePrepared();
	virtual void NodePrepared_Implementation() = 0;

	/**
	 * Calls to the Node to Process it.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Manager")
	void ProcessNode();
	virtual void ProcessNode_Implementation() = 0;

	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Manager")
	void NodeProcessed();
	virtual void NodeProcessed_Implementation() = 0;

	/**
	 * Notifies the Dialogue  that a node has been selected.
	 *
	 * @param NodeGuid The GUID of the selected node.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Manager")
	void SelectNode(const FGuid& NodeGuid);
	virtual void SelectNode_Implementation(const FGuid& NodeGuid) = 0;
	
	// --- Node Rows Loop functions ------------------------------

	/**
	 * Starts Dialogue Row execution.
	 * 
	 * ❔ Once all Dialogue Data Rows are finished, Dialogue Data is finished as well.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Manager")
	void ProcessDialogueRow();
	virtual void ProcessDialogueRow_Implementation() = 0;

	/**
	 * Function responsible for cleanup once Dialogue Row is finished.
	 * 
	 * ❔ Dialogue Data contain Dialogue Data Rows, which are individual dialogue lines, which can be skipped.
	 * ❔ Once all Dialogue Data Rows are finished, Dialogue Data is finished as well.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Manager")
	void DialogueRowProcessed(const bool bForceFinish = false);
	virtual void DialogueRowProcessed_Implementation(const bool bForceFinish = false) = 0;

	/**
	 * Skips the current dialogue row.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|Manager")
	void SkipDialogueRow();
	virtual void SkipDialogueRow_Implementation() = 0;

public:

	virtual FDialogueStartRequested& GetDialogueStartRequestedEventHandle() = 0;
	virtual FDialogueStartRequestedResult& GetDialogueStartRequestedResultEventHandle() = 0;
	virtual FDialogueEvent& GetDialogueStartedEventHandle() = 0;
	virtual FDialogueEvent& GetDialogueClosedEventHandle() = 0;
	
	virtual FDialogueContextUpdated& GetDialogueContextUpdatedEventHande() = 0;
	virtual FDialogueUserInterfaceChanged& GetDialogueUserInterfaceChangedEventHandle() = 0;

	virtual FDialogueNodeEvent& GetDialogueNodeSelectedEventHandle() = 0;

	virtual FDialogueNodeEvent& GetDialogueNodeStartedEventHandle() = 0;
	virtual FDialogueNodeEvent& GetDialogueNodeFinishedEventHandle() = 0;
	virtual FDialogueRowEvent& GetDialogueRowStartedEventHandle() = 0;
	virtual FDialogueRowEvent& GetDialogueRowFinishedEventHandle() = 0;

	virtual FDialogueFailed& GetDialogueFailedEventHandle() = 0;

	virtual FDialogueManagerStateChanged& GetDialogueManagerStateChangedEventHandle() = 0;

	virtual FDialogueWidgetCommand& GetDialogueWidgetCommandHandle() = 0;

	virtual FTimerHandle& GetDialogueRowTimerHandle() = 0;

	virtual FDialogueUISignalEvent& GetDialogueUISignalEventHandle() = 0;
};
