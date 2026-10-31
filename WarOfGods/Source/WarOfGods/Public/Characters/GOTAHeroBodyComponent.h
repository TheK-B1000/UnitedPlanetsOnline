// White Tiger Studios 2024

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GOTAHeroBodyComponent.generated.h"

class USkeletalMeshComponent;
class UGOTAHeroDefinition;

/**
 * Dresses a gameplay pawn in its hero's body (UGOTAHeroDefinition::BodyMesh).
 *
 * The server adds this to the spawned pawn and sets HeroId; HeroId replicates and every machine
 * applies the same cosmetic body locally. The pawn's own mesh is hidden but keeps animating and
 * refreshing bones, so montages, sockets, hit traces and notifies are untouched; the body follows
 * it through a runtime IK retargeter. Purely visual — no collision, no gameplay state.
 */
UCLASS(ClassGroup = (GOTA), meta = (BlueprintSpawnableComponent))
class WAROFGODS_API UGOTAHeroBodyComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGOTAHeroBodyComponent();

	/** Server: adds (or reuses) the component on Pawn and assigns the hero. Returns null if the hero has no body. */
	static UGOTAHeroBodyComponent* ApplyToPawn(APawn* Pawn, const UGOTAHeroDefinition* Hero);

	/** Server: body for HeroId, or for the default (prototype) hero when HeroId is None (modes without hero selection). */
	static UGOTAHeroBodyComponent* ApplyForHeroOrDefault(APawn* Pawn, FName HeroId);

	void SetHeroId(FName InHeroId);
	FName GetHeroId() const { return HeroId; }

	UFUNCTION(BlueprintPure, Category = "GOTA|Heroes")
	USkeletalMeshComponent* GetBodyMesh() const { return Body; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void OnRep_HeroId();

	void ApplyBody();
	void RemoveBody();
	void HideAttachedWeaponMeshes();
	void FollowBodyWithWeapons();

	UPROPERTY(ReplicatedUsing = OnRep_HeroId)
	FName HeroId;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> Body;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USkeletalMeshComponent>> BodyExtras;

	/** The pawn mesh we hid (restored if the body is removed). */
	TWeakObjectPtr<USkeletalMeshComponent> SourceMesh;
	bool bHideWeapons = false;
	float WeaponSweepTimer = 0.f;
};
