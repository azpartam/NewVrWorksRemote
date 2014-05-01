// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BlueprintEditorModule.h"
#include "BlueprintUtilities.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "Toolkits/AssetEditorManager.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "GraphEditor.h"

/////////////////////////////////////////////////////
// FCustomDebugObjectEntry - Used to pass a custom debug object override around

struct FCustomDebugObject
{
public:
	// Custom object to include, regardless of the current debugging World
	UObject* Object;

	// Override for the object name (if not empty)
	FString NameOverride;

public:
	FCustomDebugObject()
		: Object(NULL)
	{
	}

	FCustomDebugObject(UObject* InObject, const FString& InLabel)
		: Object(InObject)
		, NameOverride(InLabel)
	{
	}
};

/////////////////////////////////////////////////////
// FSelectionDetailsSummoner

#define LOCTEXT_NAMESPACE "BlueprintEditor"

struct KISMET_API FSelectionDetailsSummoner : public FWorkflowTabFactory
{
public:
	FSelectionDetailsSummoner(TSharedPtr<class FAssetEditorToolkit> InHostingApp);

	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const OVERRIDE;

	virtual FText GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const OVERRIDE
	{
		return LOCTEXT("SelectionDetailsTooltip", "The Details tab allows you see and edit properties of whatever is selected.");
	}
};

/////////////////////////////////////////////////////
// FComponentEventConstructionData

/** The structure used to construct the "Add Event" menu entries */
struct FComponentEventConstructionData
{
	// The name of the event handler to create.
	FName VariableName;
	// The template component that the handler applies to.
	TWeakObjectPtr<UActorComponent> Component;
};

/** The delegate that the caller must supply to BuildComponentActionsSubMenu that returns the currently selected items */
DECLARE_DELEGATE_OneParam(FGetSelectedObjectsDelegate, TArray<FComponentEventConstructionData>&);

/** Delegate for Node Creation Analytics */
DECLARE_DELEGATE(FNodeCreationAnalytic);

/** Describes user actions that created new node */
namespace ENodeCreateAction
{
	enum Type
	{
		MyBlueprintDragPlacement,
		PaletteDragPlacement,
		GraphContext,
		PinContext,
		Keymap
	};
}

/////////////////////////////////////////////////////
// FBlueprintEditor

/** Main Kismet asset editor */
class KISMET_API FBlueprintEditor : public IBlueprintEditor, public FGCObject, public FNotifyHook, public FTickableEditorObject, public FEditorUndoClient
{
	DECLARE_MULTICAST_DELEGATE_OneParam( FOnSetPinVisibility, SGraphEditor::EPinVisibility );

	/** A record of a warning generated by a disallowed pin connection attempt */
	struct FDisallowedPinConnection
	{
		FString PinTypeCategoryA;
		bool bPinIsArrayA;
		bool bPinIsReferenceA;
		bool bPinIsWeakPointerA;

		FString PinTypeCategoryB;
		bool bPinIsArrayB;
		bool bPinIsReferenceB;
		bool bPinIsWeakPointerB;
	};

public:
	// IToolkit interface
	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) OVERRIDE;
	// End of IToolkit interface

public:
	/**
	 * Edits the specified blueprint
	 *
	 * @param	Mode					Asset editing mode for this editor (standalone or world-centric)
	 * @param	InitToolkitHost			When Mode is WorldCentric, this is the level editor instance to spawn this editor within
	 * @param	InBlueprints			The blueprints to edit
	 * @param	bShouldOpenInDefaultsMode	If true, the editor will open in defaults editing mode
	 */
	void InitBlueprintEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, const TArray<class UBlueprint*>& InBlueprints, bool bShouldOpenInDefaultsMode);

	/*
	 * Set transactional flag on SCSNodes and its children.
	 *
	 * @param: Node reference to set transactional flag.
	 */
	static void SetSCSNodesTransactional(USCS_Node* Node);

public:
	// FAssetEditorToolkit interface
	virtual bool OnRequestClose() OVERRIDE;
	virtual void ToolkitBroughtToFront() OVERRIDE;
	// End of FAssetEditorToolkit 

	// IToolkit interface
	virtual FName GetToolkitFName() const OVERRIDE;
	virtual FText GetBaseToolkitName() const OVERRIDE;
	virtual FText GetToolkitName() const OVERRIDE;
	virtual FString GetWorldCentricTabPrefix() const OVERRIDE;
	virtual FLinearColor GetWorldCentricTabColorScale() const OVERRIDE;
	virtual bool IsBlueprintEditor() const OVERRIDE;
	// End of IToolkit interface

	// FGCObject interface
	virtual void AddReferencedObjects( FReferenceCollector& Collector ) OVERRIDE;
	// End of FGCObject interface

	// IBlueprintEditor interface
	virtual void RefreshEditors() OVERRIDE;
	virtual void JumpToHyperlink(const UObject* ObjectReference, bool bRequestRename = false) OVERRIDE;
	virtual void SummonSearchUI(bool bSetFindWithinBlueprint, FString NewSearchTerms = FString(), bool bSelectFirstResult = false) OVERRIDE;
	virtual TArray<TSharedPtr<class FSCSEditorTreeNode> > GetSelectedSCSEditorTreeNodes() const OVERRIDE;
	virtual TSharedPtr<class FSCSEditorTreeNode> FindAndSelectSCSEditorTreeNode(const UActorComponent* InComponent, bool IsCntrlDown) OVERRIDE;
	virtual int32 GetNumberOfSelectedNodes() const OVERRIDE;
	// End of IBlueprintEditor interface

	// FTickableEditorObject interface
	virtual void Tick(float DeltaTime) OVERRIDE;
	virtual bool IsTickable() const OVERRIDE { return true; }
	virtual TStatId GetStatId() const OVERRIDE;
	// End of FTickableEditorObject interface

public:
	FBlueprintEditor();

	virtual ~FBlueprintEditor();

	/** Check the Node Title is visible */
	bool IsNodeTitleVisible(const UEdGraphNode* Node, bool bRequestRename);

	/** Pan the view to center on a particular node */
	void JumpToNode(const class UEdGraphNode* Node, bool bRequestRename=false);

	/** Pan the view to center on a particular pin */
	void JumpToPin(const class UEdGraphPin* Pin);

	/** Returns a pointer to the Blueprint object we are currently editing, as long as we are editing exactly one */
	virtual UBlueprint* GetBlueprintObj() const;
	
	/**	Returns whether the editor is currently editing a single blueprint object */
	bool IsEditingSingleBlueprint() const;
	
	/** Getters for the various Kismet2 widgets */
	TSharedRef<class SKismetInspector> GetInspector() const {return Inspector.ToSharedRef();}
	TSharedRef<class SKismetInspector> GetDefaultEditor() const {return DefaultEditor.ToSharedRef();}
	TSharedRef<class SKismetDebuggingView> GetDebuggingView() const {return DebuggingView.ToSharedRef();}
	TSharedRef<class SBlueprintPalette> GetPalette() const {return Palette.ToSharedRef();}
	TSharedRef<class SWidget> GetCompilerResults() const {return CompilerResults.ToSharedRef();}
	TSharedRef<class SFindInBlueprints> GetFindResults() const {return FindResults.ToSharedRef();}
	TSharedRef<class SSCSEditor> GetSCSEditor() const {return SCSEditor.ToSharedRef();}
	TSharedPtr<class SSCSEditorViewport> GetSCSViewport() const {return SCSViewport;}
	TSharedPtr<class SMyBlueprint> GetMyBlueprintWidget() const {return MyBlueprintWidget;}
	AActor* GetPreviewActor() const;

	TSharedPtr<class FBlueprintEditorToolbar> GetToolbarBuilder() {return Toolbar;}

	/** @return the documentation location for this editor */
	virtual FString GetDocumentationLink() const OVERRIDE;

	/**	Returns whether the edited blueprint has components */
	bool CanAccessComponentsMode() const;
	
	// @todo This is a hack for now until we reconcile the default toolbar with application modes
	void RegisterToolbarTab(const TSharedRef<class FTabManager>& TabManager);
	/** Throw a simple message into the log */
	void LogSimpleMessage(const FText& MessageText);

	/** Dumps messages to the compiler log, with an option to force it to display/come to front */
	void DumpMessagesToCompilerLog(const TArray<TSharedRef<class FTokenizedMessage>>& Messages, bool bForceMessageDisplay);

	/** Returns true if in debugging mode */
	bool InDebuggingMode() const;	
	
	/** Get the currently selected set of nodes */
	TSet<UObject*> GetSelectedNodes() const;

	/** Save the current set of edited objects in the LastEditedObjects array so it will be opened next time we open K2 */
	void SaveEditedObjectState();

	/** Create new tab for each element of LastEditedObjects array */
	void RestoreEditedObjectState();

	// Request a save of the edited object state
	// This is used to delay it by one frame when triggered by a tab being closed, so it can finish closing before remembering the new state
	void RequestSaveEditedObjectState();

	/** Returns whether a graph is editable or not */
	virtual bool IsEditable(UEdGraph* InGraph) const;

	/** Returns true if in editing mode */
	bool InEditingMode() const;

	/** Returns true if able to compile */
	bool IsCompilingEnabled() const;

	/** Returns true if property editing is allowed */
	bool IsPropertyEditingEnabled() const;

	/** Returns true if the parent class is also a Blueprint */
	bool IsParentClassOfObjectABlueprint( const UBlueprint* Blueprint ) const;

	/** Returns true if the parent class of the Blueprint being edited is also a Blueprint */
	bool IsParentClassABlueprint() const;

	/** Returns true if the parent class of the Blueprint being edited is native */
	bool IsParentClassNative() const;

	/** Returns true if the parent class is native and the link to it's header can be shown*/
	bool IsNativeParentClassCodeLinkEnabled() const;

	/** Handles opening the header file of native parent class */
	void OnEditParentClassNativeCodeClicked();

	/** Called to open native function definition of the current node selection in an IDE */
	void GotoNativeFunctionDefinition();

	/** Called to check if the current selection is a native function */
	bool IsSelectionNativeFunction();

	/** Called to open native variable declaration of the current node selection in an IDE */
	void GotoNativeVariableDefinition();

	/** Called to check if the current selection is a native variable */
	bool IsSelectionNativeVariable();

	/** Returns: "(<NativeParentClass>.h)" */
	FText GetTextForNativeParentClassHeaderLink() const;

	/** Determines visibility of the native parent class manipulation buttons on the menu bar overlay */
	EVisibility GetNativeParentClassButtonsVisibility() const;

	/** Determines visibility of the standard parent class label on the menu bar overlay */
	EVisibility GetParentClassNameVisibility() const;

	/** Returns our PIE Status - SIMULATING / SERVER / CLIENT */
	FString GetPIEStatus() const;

	/**
	 * Util for finding a glyph for a graph
	 *
	 * @param Graph - The graph to evaluate
	 * @param bInLargeIcon - if true the icon returned is 22x22 pixels, else it is 16x16
	 * @return An appropriate brush to use to represent the graph, if the graph is an unknown type the function will return the default "function" glyph
	 */
	static const FSlateBrush* GetGlyphForGraph(const UEdGraph* Graph, bool bInLargeIcon = false);

	
	static FSlateBrush const* GetVarIconAndColor(UClass* VarClass, FName VarName, FSlateColor& IconColorOut);

	/** Overridable function for determining if the current mode can script */
	virtual bool IsInAScriptingMode() const;

	/** Called when Compile button is clicked */
	virtual void Compile();

	/** Calls the above function, but returns an FReply::Handled(). Used in SButtons */
	virtual FReply Compile_OnClickWithReply();

	/** Called when the refresh all nodes button is clicked */
	void RefreshAllNodes_OnClicked();

	EVisibility IsDebuggerVisible() const;

	virtual void OnActiveTabChanged(TSharedPtr<SDockTab> PreviouslyActive, TSharedPtr<SDockTab> NewlyActivated);

	/** Called when a token in a log message is clicked */
	void LogToken_OnClicked(const class IMessageToken& Token);

	void FocusInspectorOnGraphSelection(const TSet<class UObject*>& NewSelection, bool bForceRefresh = false);

	/** Variable list window calls this after it is updated */	
	void VariableListWasUpdated();

	/** Virtual override point for editing defaults; allowing more derived editors to edit something else */
	virtual void StartEditingDefaults(bool bAutoFocus = true, bool bForceRefresh = false);

	// Called by the blueprint editing app mode to focus the appropriate tabs, etc...
	void SetupViewForBlueprintEditingMode();

	// Ensures the blueprint is up to date
	void EnsureBlueprintIsUpToDate(UBlueprint* BlueprintObj);

	// Should be called when initializing any editor built off this foundation
	void CommonInitialization(const TArray<UBlueprint*>& InitBlueprints);

	// Should be called when initializing an editor that has a blueprint, after layout (tab spawning) is done
	void PostLayoutBlueprintEditorInitialization();

	/** Called when graph editor focus is changed */
	virtual void OnGraphEditorFocused(const TSharedRef<class SGraphEditor>& InGraphEditor);

	/** Enable/disable the SCS editor preview viewport */
	void EnableSCSPreview(bool bEnable);

	/** Refresh the preview viewport to reflect changes in the SCS */
	void UpdateSCSPreview(bool bUpdateNow = false);

	/** Pin visibility accessors */
	void SetPinVisibility(SGraphEditor::EPinVisibility Visibility);
	bool GetPinVisibility(SGraphEditor::EPinVisibility Visibility) const { return PinVisibility == Visibility; }
	
	/** Reparent the current blueprint */
	void ReparentBlueprint_Clicked();
	bool ReparentBlueprint_IsVisible() const;
	void ReparentBlueprint_NewParentChosen(UClass* ChosenClass);

	/** Utility function to handle all steps required to rename a newly added action */
	void RenameNewlyAddedAction(FName InActionName);

	/** Adds a new variable to this blueprint */
	void OnAddNewVariable();
	FReply OnAddNewVariable_OnClick() {OnAddNewVariable(); return FReply::Handled();}
	
	/** Adds a new local variable to the focused function graph */
	void OnAddNewLocalVariable();

	// Type of new document/graph being created by a menu item
	enum ECreatedDocumentType
	{
		CGT_NewVariable,
		CGT_NewFunctionGraph,
		CGT_NewMacroGraph,
		CGT_NewAnimationGraph,
		CGT_NewEventGraph,
		CGT_NewLocalVariable
	};

	/** Called when New Function button is clicked */
	void NewDocument_OnClicked(ECreatedDocumentType GraphType);
	FReply NewDocument_OnClick(ECreatedDocumentType GraphType) {NewDocument_OnClicked(GraphType); return FReply::Handled();}

	/** Called when New Delegate button is clicked */
	void OnAddNewDelegate();
	bool AddNewDelegateIsVisible() const;

	// Called to see if the new document menu items is visible for this type
	bool NewDocument_IsVisibleForType(ECreatedDocumentType GraphType) const;
	EVisibility NewDocument_GetVisibilityForType(ECreatedDocumentType GraphType) const
	{
		return NewDocument_IsVisibleForType(GraphType) ? EVisibility::Visible : EVisibility::Collapsed;
	}
	
	/** Clear selection across all editors */
	void ClearSelectionInAllEditors();
	
	/** Gets or sets the flag for context sensitivity in the graph action menu */
	bool& GetIsContextSensitive() {return bIsActionMenuContextSensitive;}

	/** Selection state, because all selection in this editor is mutually exclusive */
	enum ESelectionState
	{
		NoSelection,
		MyBlueprint,
		GraphPanel,
		BlueprintProps,
	};

	/** Gets the UI selection state of this editor */
	ESelectionState& GetUISelectionState() {return CurrentUISelection;}

	/** Find all instances of the selected custom event. */
	void OnFindInstancesCustomEvent();

	/** Handles spawning a graph node in the current graph using the passed in gesture */
	FReply OnSpawnGraphNodeByShortcut(FInputGesture InGesture, const FVector2D& InPosition, UEdGraph* InGraph);

	/* 
	 * Perform the actual promote to variable action on the given pin in the given blueprint.
	 *
	 * @param	InBlueprint	The blueprint in which to create the variable.
	 * @param	InTargetPin The pin on which to base the variable.
	 */
	void DoPromoteToVariable( UBlueprint* InBlueprint, UEdGraphPin* InTargetPin );		

	/**
	 * Checks for events in the argument class
	 * @param InClass	The class to check for events.
	 */
	static bool CanClassGenerateEvents( UClass* InClass );

	/** Called when node is spawned by keymap */
	void OnNodeSpawnedByKeymap();

	/** Update Node Creation mechanisms for analytics */
	void UpdateNodeCreationStats( const ENodeCreateAction::Type CreateAction );

	/** 
	 * Register a customization for interacting with the SCS editor 
	 * @param	InComponentName			The name of the component to customize behavior for
	 * @param	InCustomization			The customization instance to use
	 */
	void RegisterSCSEditorCustomization(const FName& InComponentName, TSharedPtr<class ISCSEditorCustomization> InCustomization);

	/** 
	 * Unregister a previously registered customization for interacting with the SCS editor 
	 * @param	InComponentName			The name of the component to customize behavior for
	 */
	void UnregisterSCSEditorCustomization(const FName& InComponentName);

	/** 
	 * Check to see if we can customize the SCS editor for the passed-in scene component 
	 * @param	InComponentToCustomize	The component to check to see if a customization exists
	 * @return an SCS editor customization instance, if one exists.
	 */
	TSharedPtr<class ISCSEditorCustomization> CustomizeSCSEditor(USceneComponent* InComponentToCustomize) const;

	/**
	 * Returns the currently focused graph in the Blueprint editor
	 */
	UEdGraph* GetFocusedGraph() const;

	/** Adds to a list of custom objects for debugging beyond what will automatically be found/used */
	virtual void GetCustomDebugObjects(TArray<FCustomDebugObject>& DebugList) const { }

	/** Called when a node's title is committed for a rename */
	void OnNodeTitleCommitted(const FText& NewText, ETextCommit::Type CommitInfo, UEdGraphNode* NodeBeingChanged);

	/** Called by a graph title bar to get any extra information the editor would like to display */
	virtual FString GetGraphDecorationString(UEdGraph* InGraph) const;

	/** Checks to see if the provided graph is contained within the current blueprint */
	bool IsGraphInCurrentBlueprint(UEdGraph* InGraph) const;

protected:
	
	// Zooming to fit the entire graph
	void ZoomToWindow_Clicked();
	bool CanZoomToWindow() const;

	// Zooming to fit the current selection
	void ZoomToSelection_Clicked();
	bool CanZoomToSelection() const;

	// Navigating into/out of graphs
	void NavigateToParentGraph_Clicked();
	bool CanNavigateToParentGraph() const;
	void NavigateToChildGraph_Clicked();
	bool CanNavigateToChildGraph() const;

	/** Determines visibility of the parent class manipulation buttons on the menu bar overlay */
	EVisibility ParentClassButtonsVisibility() const;

	/** Recreates the overlay on the menu bar */
	virtual void PostRegenerateMenusAndToolbars() OVERRIDE;

	/** Returns the name of the Blueprint's parent class */
	FText GetParentClassNameText() const;

	/** Handler for "Find parent class in CB" button */
	FReply OnFindParentClassInContentBrowserClicked();

	/** Handler for "Edit parent class" button */
	FReply OnEditParentClassClicked();

	/** Called to start a quick find (focus the search box in the explorer tab) */
	void FindInBlueprint_Clicked();

	/** Edit Blueprint global options */
	void EditGlobalOptions_Clicked();

	/** Called to undo the last action */
	void UndoGraphAction();

	/** Whether or not we can perform an undo of the last transacted action */
	bool CanUndoGraphAction() const;

	/** Called to redo the last undone action */
	void RedoGraphAction();

	/** Whether or not we can redo an undone action */
	bool CanRedoGraphAction() const;

	/** Called when the selection changes in the GraphEditor */
	void OnSelectedNodesChanged(const TSet<class UObject*>& NewSelection);

	/** Called when an actor is dropped onto the graph editor */
	void OnGraphEditorDropActor(const TArray< TWeakObjectPtr<AActor> >& Actors, UEdGraph* Graph, const FVector2D& DropLocation);

	/** Called when a streaming level is dropped onto the graph editor */
	void OnGraphEditorDropStreamingLevel(const TArray< TWeakObjectPtr<ULevelStreaming> >& Levels, UEdGraph* Graph, const FVector2D& DropLocation);

	/** Called to create context menu when right-clicking on graph */
	FActionMenuContent OnCreateGraphActionMenu(UEdGraph* InGraph, const FVector2D& InNodePosition, const TArray<UEdGraphPin*>& InDraggedPins, bool bAutoExpand, SGraphEditor::FActionMenuClosed InOnMenuClosed);

	/** Called from graph context menus when they close to tell the editor why they closed */
	void OnGraphActionMenuClosed(bool bActionExecuted, bool bContextSensitiveChecked, bool bGraphPinContext);

	/** Called when the Blueprint we are editing has changed */
	virtual void OnBlueprintChanged(UBlueprint* InBlueprint);

	/** Get title for Inspector 2 tab*/
	virtual FString GetDefaultEditorTitle();

	//@TODO: Should the breakpoint/watch modification operations be whole-blueprint, or current-graph?

	/** Deletes all breakpoints for the blueprint being edited */
	void ClearAllBreakpoints();

	/** Disables all breakpoints for the blueprint being edited */
	void DisableAllBreakpoints();

	/** Enables all breakpoints for the blueprint being edited */
	void EnableAllBreakpoints();

	/** Clears all watches associated with the blueprint being edited */
	void ClearAllWatches();

	bool HasAnyBreakpoints() const;
	bool HasAnyEnabledBreakpoints() const;
	bool HasAnyDisabledBreakpoints() const;
	bool HasAnyWatches() const;

	// Utility helper to get the currently hovered pin in the currently visible graph, or NULL if there isn't one
	UEdGraphPin* GetCurrentlySelectedPin() const;

	// UI Action functionality
	void OnPromoteToVariable();
	bool CanPromoteToVariable() const;

	void OnAddExecutionPin();
	bool CanAddExecutionPin() const;

	void OnRemoveExecutionPin();
	bool CanRemoveExecutionPin() const;

	void OnAddOptionPin();
	bool CanAddOptionPin() const;

	void OnRemoveOptionPin();
	bool CanRemoveOptionPin() const;

	/** Functions for handling the changing of the pin's type (PinCategory, PinSubCategory, etc) */
	FEdGraphPinType OnGetPinType(UEdGraphPin* SelectedPin) const;
	void OnChangePinType();
	void OnChangePinTypeFinished(const FEdGraphPinType& PinType, UEdGraphPin* SelectedPin);
	bool CanChangePinType() const;

	void OnAddParentNode();
	bool CanAddParentNode() const;

	void OnEnableBreakpoint();
	bool CanEnableBreakpoint() const;

	void OnToggleBreakpoint();
	bool CanToggleBreakpoint() const;

	void OnDisableBreakpoint();
	bool CanDisableBreakpoint() const;

	void OnAddBreakpoint();
	bool CanAddBreakpoint() const;

	void OnRemoveBreakpoint();
	bool CanRemoveBreakpoint() const;

	void OnCollapseNodes();
	bool CanCollapseNodes() const;

	void OnCollapseSelectionToFunction();
	bool CanCollapseSelectionToFunction() const;

	void OnCollapseSelectionToMacro();
	bool CanCollapseSelectionToMacro() const;

	void OnPromoteSelectionToFunction();
	bool CanPromoteSelectionToFunction() const;

	void OnPromoteSelectionToMacro();
	bool CanPromoteSelectionToMacro() const;

	void OnExpandNodes();
	bool CanExpandNodes() const;

	void SelectAllNodes();
	bool CanSelectAllNodes() const;

	void DeleteSelectedNodes();
	bool CanDeleteNodes() const;

	void DeleteSelectedDuplicatableNodes();

	void CutSelectedNodes();
	bool CanCutNodes() const;

	void CopySelectedNodes();
	bool CanCopyNodes() const;
	
	/** Paste on graph at specific location */
	virtual void PasteNodesHere(class UEdGraph* DestinationGraph, const FVector2D& GraphLocation) OVERRIDE;
	
	void PasteNodes();
	virtual bool CanPasteNodes() const OVERRIDE;

	void DuplicateNodes();
	bool CanDuplicateNodes() const;

	void OnSelectReferenceInLevel();
	bool CanSelectReferenceInLevel() const;

	void OnAssignReferencedActor();
	bool CanAssignReferencedActor() const;

	void OnStartWatchingPin();
	bool CanStartWatchingPin() const;
	
	void OnStopWatchingPin();
	bool CanStopWatchingPin() const;

	/**  BEGIN PERSONA related callback functions */
	virtual void OnSelectBone() {};
	virtual bool CanSelectBone() const { return false; }

	virtual void OnAddPosePin() {};
	virtual bool CanAddPosePin() const { return false; }

	virtual void OnMergeAnimStateTransitions() {};
	virtual bool CanMergeAnimStateTransitions() const { return false;}

	virtual void OnRemovePosePin() {};
	virtual bool CanRemovePosePin() const { return false; }

	// convert functions between evaluator and player
	virtual void OnConvertToSequenceEvaluator() {};
	virtual void OnConvertToSequencePlayer() {};
	virtual void OnConvertToBlendSpaceEvaluator() {};
	virtual void OnConvertToBlendSpacePlayer() {};

	// Opens the associated asset of the selected nodes
	virtual void OnOpenRelatedAsset() {};
	/** END PERSONA related callback functions */

	void ToggleSaveIntermediateBuildProducts();
	bool GetSaveIntermediateBuildProducts() const;

	void OnListObjectsReferencedByClass();
	void OnListObjectsReferencedByBlueprint();
	void OnRepairCorruptedBlueprint();

	void OnNodeDoubleClicked(class UEdGraphNode* Node);

	virtual void OnEditTabClosed(TSharedRef<SDockTab> Tab);

	virtual bool GetBoundsForSelectedNodes(class FSlateRect& Rect, float Padding);

	/**
	 * Pulls out the pins to use as a template when collapsing a selection to a function with a custom event involved.
	 *
	 * @param InCustomEvent				The custom event used as a template
	 * @param InGatewayNode				The node replacing the selection of nodes
	 * @param InEntryNode				The entry node in the graph
	 * @param InResultNode				The result node in the graph
	 * @param InCollapsableNodes		The selection of nodes being collapsed
	 */
	void ExtractEventTemplateForFunction(class UK2Node_CustomEvent* InCustomEvent, UEdGraphNode* InGatewayNode, class UK2Node_EditablePinBase* InEntryNode, class UK2Node_EditablePinBase* InResultNode, TSet<UEdGraphNode*>& InCollapsableNodes);

	/**
	 * Collapses a selection of nodes into a graph for composite, function, or macro nodes.
	 *
	 * @param InGatewayNode				The node replacing the selection of nodes
	 * @param InEntryNode				The entry node in the graph
	 * @param InResultNode				The result node in the graph
	 * @param InSourceGraph				The graph the selection is from
	 * @param InDestinationGraph		The destination graph to move the selected nodes to
	 * @param InCollapsableNodes		The selection of nodes being collapsed
	 */
	void CollapseNodesIntoGraph(UEdGraphNode* InGatewayNode, class UK2Node_EditablePinBase* InEntryNode, class UK2Node_EditablePinBase* InResultNode, UEdGraph* InSourceGraph, UEdGraph* InDestinationGraph, TSet<UEdGraphNode*>& InCollapsableNodes);

	/** Called when a selection of nodes are being collapsed into a sub-graph */
	void CollapseNodes(TSet<class UEdGraphNode*>& InCollapsableNodes);

	/** Called when a selection of nodes are being collapsed into a function */
	UEdGraph* CollapseSelectionToFunction(TSet<class UEdGraphNode*>& InCollapsableNodes, UEdGraphNode*& OutFunctionNode);

	/** Called when a selection of nodes are being collapsed into a macro */
	UEdGraph* CollapseSelectionToMacro(TSet<class UEdGraphNode*>& InCollapsableNodes, UEdGraphNode*& OutMacroNode);

	/**
	 * Called when a selection of nodes is being collapsed into a function
	 *
	 * @param InSelection		The selection to check
	 *
	 * @return					Returns TRUE if the selection can be promoted to a function
	 */
	bool CanCollapseSelectionToFunction(TSet<class UEdGraphNode*>& InSelection) const;

	/**
	 * Called when a selection of nodes is being collapsed into a macro
	 *
	 * @param InSelection		The selection to check
	 *
	 * @return					Returns TRUE if the selection can be promoted to a macro
	 */
	bool CanCollapseSelectionToMacro(TSet<class UEdGraphNode*>& InSelection) const;

	/**
	 * Expands passed in node */
	static void ExpandNode(UEdGraphNode* InNodeToExpand, UEdGraph* InSourceGraph, TSet<UEdGraphNode*>& OutExpandedNodes);

	/** Start editing the defaults for this blueprint */
	void OnStartEditingDefaultsClicked();

	/** Creates the widgets that go into the tabs (note: does not create the tabs themselves) **/
	virtual void CreateDefaultTabContents(const TArray<UBlueprint*>& InBlueprints);

	/** Create Default Commands **/
	virtual void CreateDefaultCommands();

	/** Called when DeleteUnusedVariables button is clicked */
	void DeleteUnusedVariables_OnClicked();

	/** Called when Find In Blueprints menu is opened is clicked */
	void FindInBlueprints_OnClicked();

	// FNotifyHook interface
	virtual void NotifyPreChange( UProperty* PropertyAboutToChange ) OVERRIDE;
	virtual void NotifyPostChange( const FPropertyChangedEvent& PropertyChangedEvent, UProperty* PropertyThatChanged) OVERRIDE;
	// End of FNotifyHook interface

	/** Callback when properties have finished being handled */
	void OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent);

	/** the string to show for edit defaults*/
	static FString DefaultEditString();

	/** On starting to rename node */
	void OnRenameNode();
	bool CanRenameNodes() const;

	/* Renames a GraphNode */
	void RenameGraph(class UEdGraphNode* GraphNode, const FString& NewName);

	/** Called when a node's title is being committed for a rename so it can be verified */
	bool OnNodeVerifyTitleCommit(const FText& NewText, UEdGraphNode* NodeBeingChanged);

	/**Load macro & function blueprint libraries from asset registry*/
	void LoadLibrariesFromAssetRegistry();

	// Begin FEditorUndoClient Interface
	virtual void	PostUndo(bool bSuccess) OVERRIDE;
	virtual void	PostRedo(bool bSuccess) OVERRIDE;
	// End of FEditorUndoClient

	virtual void AnalyticsTrackNewNode( FName NodeClass, FName NodeType ) OVERRIDE;

	/** Get graph appearance */
	virtual FGraphAppearanceInfo GetGraphAppearance() const;

private:

	/* User wants to edit tunnel via function editor */
	void OnEditTunnel();

	/* Create comment node on graph */
	void OnCreateComment();

	// Create new graph editor widget for the supplied document container
	TSharedRef<SGraphEditor> CreateGraphEditorWidget(TSharedRef<class FTabInfo> InTabInfo, class UEdGraph* InGraph);

	/** Helper to move focused graph when clicking on graph breadcrumb */
	void OnChangeBreadCrumbGraph( class UEdGraph* InGraph);

	/** Function to check whether the give graph is a subgraph */
	static bool IsASubGraph( const class UEdGraph* GraphPtr );

	/** Callback when a token is clicked on in the compiler results log */
	void OnLogTokenClicked(const TSharedRef<class IMessageToken>& Token);

	/** Helper function to navigate the current tab */
	void NavigateTab(FDocumentTracker::EOpenDocumentCause InCause);

	/** Find all references of the selected variable. */
	void OnFindVariableReferences();

	/** Checks if we can currently find all references of the variable selection. */
	bool CanFindVariableReferences();

	/** Called when the user generates a warning tooltip because a connection was invalid */
	void OnDisallowedPinConnection(const class UEdGraphPin* PinA, const class UEdGraphPin* PinB);

	/**
	 * Checks to see if the focused Graph-Editor is focused on a animation graph or not.
	 * 
	 * @return True if GraphEdPtr's current graph is an animation graph, false if not.
	 */
	bool IsEditingAnimGraph() const;

public://@TODO
	TSharedPtr<FDocumentTracker> DocumentManager;

protected:

	// Should intermediate build products be saved when recompiling?
	bool bSaveIntermediateBuildProducts;

	/** Currently focused graph editor */
	TWeakPtr<class SGraphEditor> FocusedGraphEdPtr;





	
	// Factory that spawns graph editors; used to look up all tabs spawned by it.
	TWeakPtr<FDocumentTabFactory> GraphEditorTabFactoryPtr;

	/** User-defined enumerators to keep loaded */
	TSet<TWeakObjectPtr<UUserDefinedEnum>> UserDefinedEnumerators;

	/** User-defined structures to keep loaded */
	TSet<TWeakObjectPtr<UUserDefinedStruct>> UserDefinedStructures;
	
	/** Macro/function libraries to keep loaded */
	TArray<UBlueprint*> StandardLibraries;

	/** SCS editor */
	TSharedPtr<class SSCSEditor> SCSEditor;

	/** Viewport widget */
	TSharedPtr<class SSCSEditorViewport> SCSViewport;

	/** Node inspector widget */
	TSharedPtr<class SKismetInspector> Inspector;

	/** defaults inspector widget */
	TSharedPtr<class SKismetInspector> DefaultEditor;

	/** Debugging window (watches, breakpoints, etc...) */
	TSharedPtr<class SKismetDebuggingView> DebuggingView;

	/** Palette of all classes with funcs/vars */
	TSharedPtr<class SBlueprintPalette> Palette;

	/** All of this blueprints' functions and variables */
	TSharedPtr<class SMyBlueprint> MyBlueprintWidget;
	
	/** Compiler results log, with the log listing that it reflects */
	TSharedPtr<class SWidget> CompilerResults;
	TSharedPtr<class IMessageLogListing> CompilerResultsListing;
	
	/** Find results log as well as the search filter */
	TSharedPtr<class SFindInBlueprints> FindResults;

	/** Reference to owner of the current popup */
	TWeakPtr<class SWindow> NameEntryPopupWindow;

	/** Reference to helper object to validate names in popup */
	TSharedPtr<class INameValidatorInterface> NameEntryValidator;

	/** Reference to owner of the pin type change popup */
	TWeakPtr<class SWindow> PinTypeChangePopupWindow;

	/** The toolbar builder class */
	TSharedPtr<class FBlueprintEditorToolbar> Toolbar;

	FOnSetPinVisibility OnSetPinVisibility;

	/** Has someone requested a deferred update of the saved document state? */
	bool bRequestedSavingOpenDocumentState;

	/** Did we update the blueprint when it opened */
	bool bBlueprintModifiedOnOpen;

	/** Whether to hide unused pins or not */
	SGraphEditor::EPinVisibility PinVisibility;

	/** Whether the graph action menu should be sensitive to the pins dragged off of */
	bool bIsActionMenuContextSensitive;
	
	/** The current UI selection state of this editor */
	ESelectionState CurrentUISelection;

	/** Whether we are already in the process of closing this editor */
	bool bEditorMarkedAsClosed;
public:
	// Tries to open the specified graph and bring it's document to the front (note: this can return NULL)
	TSharedPtr<SGraphEditor> OpenGraphAndBringToFront(UEdGraph* Graph);

	//@TODO: To be moved/merged
	TSharedPtr<SDockTab> OpenDocument(UObject* DocumentID, FDocumentTracker::EOpenDocumentCause Cause);

	/** Finds the tab associated with the specified asset, and closes if it is open */
	void CloseDocumentTab(UObject* DocumentID);

	// Finds any open tabs containing the specified document and adds them to the specified array; returns true if at least one is found
	bool FindOpenTabsContainingDocument(UObject* DocumentID, /*inout*/ TArray< TSharedPtr<SDockTab> >& Results);


public:
	/** Broadcasts a notification whenever the editor needs associated controls to refresh */
	DECLARE_EVENT ( FBlueprintEditor, FOnRefreshEvent );
	FOnRefreshEvent& OnRefresh() { return RefreshEvent; }

private:
	/** Notification used whenever the editor wants associated controls to refresh. */
	FOnRefreshEvent RefreshEvent;

	/** Broadcast notification for associated controls to update */
	void BroadcastRefresh() { RefreshEvent.Broadcast(); }

	/** Command list for the graph editor */
	TSharedPtr<FUICommandList> GraphEditorCommands;

	/** Structure to contain editor usage analytics */
	struct FAnalyticsStatistics
	{
		/** Stats collected about graph action menu usage */
		int32 GraphActionMenusNonCtxtSensitiveExecCount;
		int32 GraphActionMenusCtxtSensitiveExecCount;
		int32 GraphActionMenusCancelledCount;

		/** Stats collection about user node creation actions */
		int32 MyBlueprintNodeDragPlacementCount;
		int32 PaletteNodeDragPlacementCount;
		int32 NodeGraphContextCreateCount;
		int32 NodePinContextCreateCount;
		int32 NodeKeymapCreateCount;
		int32 NodePasteCreateCount;

		/** New node instance information */
		struct FNodeDetails
		{
			FName NodeClass;
			int32 Instances;
		};

		/** Stats collected about class/type/instances of user created nodes */
		TMap<FName,FNodeDetails> CreatedNodeTypes;

		/** Stats collected about warning tooltips */
		TArray<FDisallowedPinConnection> GraphDisallowedPinConnections;
	};

	/** analytics statistics for the Editor */ 
	FAnalyticsStatistics AnalyticsStats;

	/** Customizations for the SCS editor */
	TMap< FName, TSharedPtr<ISCSEditorCustomization> > SCSEditorCustomizations;
};

#undef LOCTEXT_NAMESPACE