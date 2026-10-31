// White Tiger Studios 2024

#include "UI/GOTAEditorUIBuilderLibrary.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/WidgetTree.h"
#include "Components/ContentWidget.h"
#include "Components/PanelWidget.h"
#include "Engine/Font.h"
#include "Engine/FontFace.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprint.h"
#include "EdGraphUtilities.h"
#include "Misc/FileHelper.h"
#include "Engine/Blueprint.h"
#include "Engine/LevelScriptBlueprint.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"

DEFINE_LOG_CATEGORY_STATIC(LogGOTAEditorUI, Log, All);

bool UGOTAEditorUIBuilderLibrary::ResetWidgetTree(UObject* WidgetBlueprint)
{
	UWidgetBlueprint* WBP = Cast<UWidgetBlueprint>(WidgetBlueprint);
	if (!WBP || !WBP->WidgetTree)
	{
		return false;
	}
	WBP->Modify();
	TArray<UWidget*> All;
	WBP->WidgetTree->GetAllWidgets(All);
	for (UWidget* Widget : All)
	{
		// Rename out of the way so names can be reused by the rebuild.
		Widget->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional);
	}
	WBP->WidgetTree->RootWidget = nullptr;
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WBP);
	return true;
}

UWidget* UGOTAEditorUIBuilderLibrary::FindWidget(UObject* WidgetBlueprint, FName WidgetName)
{
	const UWidgetBlueprint* WBP = Cast<UWidgetBlueprint>(WidgetBlueprint);
	return (WBP && WBP->WidgetTree) ? WBP->WidgetTree->FindWidget(WidgetName) : nullptr;
}

UWidget* UGOTAEditorUIBuilderLibrary::AddWidget(UObject* WidgetBlueprint, UClass* WidgetClass, FName WidgetName, FName ParentName)
{
	UWidgetBlueprint* WBP = Cast<UWidgetBlueprint>(WidgetBlueprint);
	if (!WBP || !WBP->WidgetTree || !WidgetClass || !WidgetClass->IsChildOf(UWidget::StaticClass()))
	{
		UE_LOG(LogGOTAEditorUI, Error, TEXT("AddWidget: invalid args (%s, %s)"), *GetNameSafe(WidgetBlueprint), *GetNameSafe(WidgetClass));
		return nullptr;
	}
	if (WBP->WidgetTree->FindWidget(WidgetName))
	{
		UE_LOG(LogGOTAEditorUI, Error, TEXT("AddWidget: duplicate name %s in %s"), *WidgetName.ToString(), *WBP->GetName());
		return nullptr;
	}

	UWidget* Widget = WBP->WidgetTree->ConstructWidget<UWidget>(WidgetClass, WidgetName);
	if (!Widget)
	{
		return nullptr;
	}
	Widget->bIsVariable = true;
	Widget->SetFlags(RF_Transactional);

	if (ParentName.IsNone())
	{
		WBP->WidgetTree->RootWidget = Widget;
		return Widget;
	}

	UWidget* Parent = WBP->WidgetTree->FindWidget(ParentName);
	if (UPanelWidget* Panel = Cast<UPanelWidget>(Parent))
	{
		if (!Panel->CanAddMoreChildren())
		{
			UE_LOG(LogGOTAEditorUI, Error, TEXT("AddWidget: %s cannot take more children"), *ParentName.ToString());
			return nullptr;
		}
		Panel->AddChild(Widget);
		return Widget;
	}
	UE_LOG(LogGOTAEditorUI, Error, TEXT("AddWidget: parent %s is not a panel"), *ParentName.ToString());
	return nullptr;
}

bool UGOTAEditorUIBuilderLibrary::CompileWidgetBlueprint(UObject* WidgetBlueprint)
{
	UWidgetBlueprint* WBP = Cast<UWidgetBlueprint>(WidgetBlueprint);
	if (!WBP)
	{
		return false;
	}
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WBP);
	FKismetEditorUtilities::CompileBlueprint(WBP);
	const bool bOk = WBP->Status != BS_Error;
	UE_LOG(LogGOTAEditorUI, Log, TEXT("Compiled %s: %s"), *WBP->GetName(), bOk ? TEXT("OK") : TEXT("ERROR"));
	return bOk;
}

int32 UGOTAEditorUIBuilderLibrary::ClearLevelScriptGraphs(UObject* WorldObject)
{
	UWorld* World = Cast<UWorld>(WorldObject);
	ULevel* Level = World ? World->PersistentLevel.Get() : nullptr;
	ULevelScriptBlueprint* Script = Level ? Level->GetLevelScriptBlueprint(true) : nullptr;
	if (!Script)
	{
		return 0;
	}
	Script->Modify();
	int32 Removed = 0;
	for (UEdGraph* Graph : Script->UbergraphPages)
	{
		if (!Graph)
		{
			continue;
		}
		const TArray<UEdGraphNode*> Nodes = Graph->Nodes;
		for (UEdGraphNode* Node : Nodes)
		{
			if (!Node)
			{
				continue;
			}
			UE_LOG(LogGOTAEditorUI, Log, TEXT("Level script node removed: %s"), *Node->GetNodeTitle(ENodeTitleType::ListView).ToString());
			FBlueprintEditorUtils::RemoveNode(Script, Node, true);
			++Removed;
		}
	}
	FKismetEditorUtilities::CompileBlueprint(Script);
	Level->MarkPackageDirty();
	return Removed;
}

int32 UGOTAEditorUIBuilderLibrary::ExportBlueprintGraphs(UObject* BlueprintObject, const FString& OutDir)
{
	UBlueprint* BP = Cast<UBlueprint>(BlueprintObject);
	if (!BP)
	{
		return 0;
	}
	TArray<UEdGraph*> Graphs;
	BP->GetAllGraphs(Graphs);
	int32 Written = 0;
	for (UEdGraph* Graph : Graphs)
	{
		if (!Graph)
		{
			continue;
		}
		TSet<UObject*> Nodes;
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			Nodes.Add(Node);
		}
		FString Text;
		FEdGraphUtilities::ExportNodesToText(Nodes, Text);
		const FString File = OutDir / FString::Printf(TEXT("%s__%s.txt"), *BP->GetName(), *Graph->GetName());
		if (FFileHelper::SaveStringToFile(Text, *File))
		{
			++Written;
		}
	}
	return Written;
}

UFont* UGOTAEditorUIBuilderLibrary::CreateCompositeFont(const FString& PackagePath, const FString& AssetName, const TMap<FName, UFontFace*>& Typefaces)
{
	const FString PackageName = PackagePath / AssetName;
	UPackage* Package = CreatePackage(*PackageName);
	if (!Package)
	{
		return nullptr;
	}
	UFont* Font = FindObject<UFont>(Package, *AssetName);
	if (!Font)
	{
		Font = NewObject<UFont>(Package, *AssetName, RF_Public | RF_Standalone | RF_Transactional);
		FAssetRegistryModule::AssetCreated(Font);
	}
	Font->Modify();
	Font->FontCacheType = EFontCacheType::Runtime;
	FCompositeFont& Composite = Font->GetMutableInternalCompositeFont();
	Composite.DefaultTypeface.Fonts.Reset();
	for (const TPair<FName, UFontFace*>& Pair : Typefaces)
	{
		if (Pair.Value)
		{
			FTypefaceEntry& Entry = Composite.DefaultTypeface.Fonts.Emplace_GetRef(Pair.Key);
			Entry.Font = FFontData(Pair.Value);
		}
	}
	Font->PostEditChange();
	Package->MarkPackageDirty();
	return Font;
}
#endif
