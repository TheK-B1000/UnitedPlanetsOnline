// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GOTAEditorUIBuilderLibrary.generated.h"

class UWidget;
class UFontFace;
class UFont;

/**
 * Editor-only authoring helpers used by Tools/GOTA_BuildMenuUI.py.
 * Python cannot reach a WidgetBlueprint's WidgetTree, so these build real, designer-editable
 * WBP hierarchies (the result is ordinary UMG — open and edit it in the Widget Designer).
 * Compiled out of game builds.
 */
UCLASS()
class WAROFGODS_API UGOTAEditorUIBuilderLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	/** Removes every widget from the blueprint's tree (rebuild from scratch). */
	UFUNCTION(BlueprintCallable, Category = "GOTA|EditorUI")
	static bool ResetWidgetTree(UObject* WidgetBlueprint);

	/**
	 * Constructs a widget (any UWidget subclass, including other WBP classes) named WidgetName.
	 * ParentName None = becomes the root. Otherwise added to that panel/content widget.
	 * Returns the template widget; configure it and its Slot from Python.
	 */
	UFUNCTION(BlueprintCallable, Category = "GOTA|EditorUI")
	static UWidget* AddWidget(UObject* WidgetBlueprint, UClass* WidgetClass, FName WidgetName, FName ParentName);

	UFUNCTION(BlueprintCallable, Category = "GOTA|EditorUI")
	static UWidget* FindWidget(UObject* WidgetBlueprint, FName WidgetName);

	/** Structural refresh + compile. Returns false if the compiler reports errors. */
	UFUNCTION(BlueprintCallable, Category = "GOTA|EditorUI")
	static bool CompileWidgetBlueprint(UObject* WidgetBlueprint);

	/**
	 * Removes every node from a level's Level Blueprint event graphs and recompiles it.
	 * Used once on L_GOTA_MainMenu, which was seeded from MainMenu.umap and inherited a level script
	 * that spawns the legacy WBP_MainMenu. Logs each removed node. Returns the number removed.
	 */
	UFUNCTION(BlueprintCallable, Category = "GOTA|EditorUI")
	static int32 ClearLevelScriptGraphs(UObject* World);

	/** Audit: writes every graph of a Blueprint as node text (same format as copy/paste) into OutDir. */
	UFUNCTION(BlueprintCallable, Category = "GOTA|EditorUI")
	static int32 ExportBlueprintGraphs(UObject* Blueprint, const FString& OutDir);

	/** Creates (or updates) a runtime-cached UFont asset from FontFace assets: {"Regular": Face,...}. */
	/** Editor: synchronously builds the navmesh of the current editor world (saved with the level). Returns tile count > 0. */
	UFUNCTION(BlueprintCallable, Category = "GOTA|EditorUI")
	static bool BuildEditorNavigation();

	/**
	 * Removes call nodes whose target function no longer exists (e.g. engine APIs removed since the asset
	 * was made), recompiles and saves the Blueprint. Returns the number of nodes removed (-1 on failure).
	 */
	UFUNCTION(BlueprintCallable, Category = "GOTA|EditorUI")
	static int32 RemoveDeadFunctionCalls(UBlueprint* Blueprint, FName FunctionName);

	/**
	 * Editor: spawns a box-brush volume (NavMeshBoundsVolume, BlockingVolume, AudioVolume, …) in the
	 * current editor world. Python cannot build brush geometry, so volumes are created here.
	 */
	UFUNCTION(BlueprintCallable, Category = "GOTA|EditorUI")
	static AActor* SpawnBoxVolume(TSubclassOf<AActor> VolumeClass, FVector Center, FVector Size, FRotator Rotation, FString Label);

	UFUNCTION(BlueprintCallable, Category = "GOTA|EditorUI")
	static UFont* CreateCompositeFont(const FString& PackagePath, const FString& AssetName, const TMap<FName, UFontFace*>& Typefaces);
#endif
};
