// White Tiger Studios 2024

#include "Characters/GOTAHeroBodyComponent.h"
#include "Characters/GOTAHeroDefinition.h"
#include "Characters/GOTAHeroRegistry.h"
#include "Characters/GOTARetargetAnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "Retargeter/IKRetargeter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GOTAHeroBodyComponent)

DEFINE_LOG_CATEGORY_STATIC(LogGOTAHeroBody, Log, All);

UGOTAHeroBodyComponent::UGOTAHeroBodyComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.5f; // only used to re-hide late-equipped weapon meshes
	SetIsReplicatedByDefault(true);
}

UGOTAHeroBodyComponent* UGOTAHeroBodyComponent::ApplyToPawn(APawn* Pawn, const UGOTAHeroDefinition* Hero)
{
	if (!Pawn || !Hero || Hero->BodyMesh.IsNull() || Hero->BodyRetargeter.IsNull() || !Pawn->HasAuthority())
	{
		return nullptr;
	}
	UGOTAHeroBodyComponent* Comp = Pawn->FindComponentByClass<UGOTAHeroBodyComponent>();
	if (!Comp)
	{
		Comp = NewObject<UGOTAHeroBodyComponent>(Pawn, TEXT("GOTAHeroBody"));
		Comp->RegisterComponent();
		Pawn->AddInstanceComponent(Comp);
	}
	Comp->SetHeroId(Hero->HeroId);
	return Comp;
}

void UGOTAHeroBodyComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGOTAHeroBodyComponent, HeroId);
}

void UGOTAHeroBodyComponent::SetHeroId(FName InHeroId)
{
	if (HeroId == InHeroId && Body)
	{
		return;
	}
	HeroId = InHeroId;
	ApplyBody();
}

void UGOTAHeroBodyComponent::OnRep_HeroId()
{
	ApplyBody();
}

void UGOTAHeroBodyComponent::BeginPlay()
{
	Super::BeginPlay();
	if (!HeroId.IsNone() && !Body)
	{
		ApplyBody();
	}
}

void UGOTAHeroBodyComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveBody();
	Super::EndPlay(EndPlayReason);
}

void UGOTAHeroBodyComponent::ApplyBody()
{
	RemoveBody();
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	const UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const UGOTAHeroRegistry* Registry = GI ? GI->GetSubsystem<UGOTAHeroRegistry>() : nullptr;
	const UGOTAHeroDefinition* Hero = Registry ? Registry->FindHero(HeroId) : nullptr;
	if (!Character || !Character->GetMesh() || !Hero)
	{
		return;
	}
	USkeletalMesh* BodyAsset = Hero->BodyMesh.LoadSynchronous();
	UIKRetargeter* Retargeter = Hero->BodyRetargeter.LoadSynchronous();
	if (!BodyAsset || !Retargeter)
	{
		UE_LOG(LogGOTAHeroBody, Warning, TEXT("Hero %s: body or retargeter failed to load — keeping the pawn mesh"), *HeroId.ToString());
		return;
	}

	USkeletalMeshComponent* PawnMesh = Character->GetMesh();
	Body = NewObject<USkeletalMeshComponent>(Character, TEXT("GOTAHeroBodyMesh"));
	Body->SetupAttachment(PawnMesh); // the retarget node reads its pose from the attach parent
	Body->SetSkeletalMeshAsset(BodyAsset);
	Body->SetRelativeScale3D(FVector(Hero->BodyScale));
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->SetGenerateOverlapEvents(false);
	Body->SetCanEverAffectNavigation(false);
	Body->bReceivesDecals = false;
	Body->SetAnimInstanceClass(UGOTARetargetAnimInstance::StaticClass());
	Body->RegisterComponent();
	Character->AddInstanceComponent(Body);
	Body->AddTickPrerequisiteComponent(PawnMesh); // copy this frame's pose, not last frame's
	if (UGOTARetargetAnimInstance* Anim = Cast<UGOTARetargetAnimInstance>(Body->GetAnimInstance()))
	{
		Anim->Retargeter = Retargeter;
	}

	// Hide the combat mesh but keep it evaluating: sockets, traces and notifies depend on it.
	PawnMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	PawnMesh->SetHiddenInGame(true, false);
	SourceMesh = PawnMesh;
	bHideWeapons = Hero->bHideAttachedWeaponMeshes;
	HideAttachedWeaponMeshes();
	UE_LOG(LogGOTAHeroBody, Log, TEXT("Hero %s: body %s applied to %s (scale %.2f, %s)"), *HeroId.ToString(), *GetNameSafe(BodyAsset),
		*GetNameSafe(Character), Hero->BodyScale, Character->HasAuthority() ? TEXT("authority") : TEXT("remote"));
}

void UGOTAHeroBodyComponent::RemoveBody()
{
	if (Body)
	{
		Body->DestroyComponent();
		Body = nullptr;
	}
	if (USkeletalMeshComponent* PawnMesh = SourceMesh.Get())
	{
		PawnMesh->SetHiddenInGame(false, false);
	}
	SourceMesh.Reset();
}

void UGOTAHeroBodyComponent::HideAttachedWeaponMeshes()
{
	AActor* Owner = GetOwner();
	if (!bHideWeapons || !Owner || !Body)
	{
		return;
	}
	// Weapon actors attached to the pawn (the shared prototype sword): render-hidden only.
	TArray<AActor*> Attached;
	Owner->GetAttachedActors(Attached, true, true);
	for (AActor* Actor : Attached)
	{
		if (IsValid(Actor) && !Actor->IsA<APawn>() && !Actor->IsHidden())
		{
			Actor->SetActorHiddenInGame(true);
		}
	}
	// Any other mesh component on the pawn (e.g. a weapon mesh component on BP_Player), wherever it is attached.
	TArray<UMeshComponent*> Meshes;
	Owner->GetComponents<UMeshComponent>(Meshes);
	for (UMeshComponent* Mesh : Meshes)
	{
		if (Mesh && Mesh != Body && Mesh != SourceMesh.Get() && Mesh->GetAttachParent() != Body
			&& (Mesh->IsA<UStaticMeshComponent>() || Mesh->IsA<USkeletalMeshComponent>()) && !Mesh->bHiddenInGame)
		{
			Mesh->SetHiddenInGame(true, false);
			UE_LOG(LogGOTAHeroBody, Log, TEXT("Hero %s: hid %s"), *HeroId.ToString(), *Mesh->GetName());
		}
	}
}

void UGOTAHeroBodyComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	HideAttachedWeaponMeshes();
}
