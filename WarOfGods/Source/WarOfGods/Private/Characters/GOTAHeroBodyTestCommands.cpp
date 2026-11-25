// White Tiger Studios 2024
// Development-only checks for hero bodies: where the visible hands, kept weapons and spawned spells sit
// relative to the hidden gameplay mesh. Runs on whichever machine executes it (server or client).

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Engine/SkeletalMesh.h"

#if !UE_BUILD_SHIPPING

#include "Characters/GOTAHeroBodyComponent.h"
#include "Characters/GOTARetargetAnimInstance.h"
#include "Combat/GOTACombatantObserver.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "HAL/IConsoleManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraActor.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogGOTABodyTest, Log, All);

namespace GOTABodyTest
{
	static UWorld* GameWorld()
	{
		for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
		{
			if (Ctx.World() && (Ctx.WorldType == EWorldType::Game || Ctx.WorldType == EWorldType::PIE))
			{
				return Ctx.World();
			}
		}
		return nullptr;
	}

	static const TCHAR* NetName(const UWorld* W)
	{
		switch (W->GetNetMode())
		{
		case NM_Client: return TEXT("client");
		case NM_ListenServer: return TEXT("listen-server");
		case NM_DedicatedServer: return TEXT("dedicated");
		default: return TEXT("standalone");
		}
	}

	static float BoneGap(USkeletalMeshComponent* A, USkeletalMeshComponent* B, FName Bone)
	{
		if (!A || !B || A->GetBoneIndex(Bone) == INDEX_NONE || B->GetBoneIndex(Bone) == INDEX_NONE)
		{
			return -1.f;
		}
		return FVector::Dist(A->GetBoneLocation(Bone), B->GetBoneLocation(Bone));
	}

	/** gota.Test.Body: one line per dressed pawn, plus one per kept weapon. */
	static void Body(const TArray<FString>&)
	{
		UWorld* W = GameWorld();
		if (!W)
		{
			return;
		}
		for (TActorIterator<ACharacter> It(W); It; ++It)
		{
			ACharacter* C = *It;
			UGOTAHeroBodyComponent* Comp = C->FindComponentByClass<UGOTAHeroBodyComponent>();
			USkeletalMeshComponent* BodyMesh = Comp ? Comp->GetBodyMesh() : nullptr;
			USkeletalMeshComponent* Gameplay = C->GetMesh();
			int32 VisibleMeshes = 0;
			TArray<USkeletalMeshComponent*> Skels;
			C->GetComponents<USkeletalMeshComponent>(Skels);
			for (USkeletalMeshComponent* S : Skels)
			{
				VisibleMeshes += (S->IsVisible() && !S->bHiddenInGame && S->GetSkeletalMeshAsset()) ? 1 : 0;
			}
			UE_LOG(LogGOTABodyTest, Log, TEXT("MOVE [%s] %s walk=%.0f speed=%.0f"), NetName(W), *C->GetName(), C->GetCharacterMovement() ? C->GetCharacterMovement()->MaxWalkSpeed : -1.f, C->GetVelocity().Size2D());
			UE_LOG(LogGOTABodyTest, Log, TEXT("BODY [%s] %s hero=%s body=%s bodyVisible=%d gameplayMeshHidden=%d visibleSkelMeshes=%d handGap L=%.1f R=%.1f"),
				NetName(W), *C->GetName(), Comp ? *Comp->GetHeroId().ToString() : TEXT("-"),
				BodyMesh ? *GetNameSafe(BodyMesh->GetSkeletalMeshAsset()) : TEXT("none"),
				BodyMesh && BodyMesh->IsVisible() && !BodyMesh->bHiddenInGame ? 1 : 0,
				Gameplay && (Gameplay->bHiddenInGame || !Gameplay->IsVisible()) ? 1 : 0, VisibleMeshes,
				BoneGap(Gameplay, BodyMesh, TEXT("hand_l")), BoneGap(Gameplay, BodyMesh, TEXT("hand_r")));

			// Extra meshes (hair...): anim instance, post-process instance, ponytail tip in head space.
			for (USkeletalMeshComponent* S : Skels)
			{
				if (!S || S == BodyMesh || S == Gameplay || S->GetAttachParent() != BodyMesh)
				{
					continue;
				}
				FVector Tip = FVector::ZeroVector;
				const FName TipBone(TEXT("C_Ponytail_B_Joint7_Jnt"));
				if (S->GetBoneIndex(TipBone) != INDEX_NONE && S->GetBoneIndex(TEXT("head")) != INDEX_NONE)
				{
					Tip = S->GetBoneTransform(TEXT("head")).InverseTransformPosition(S->GetBoneLocation(TipBone));
				}
				UE_LOG(LogGOTABodyTest, Log, TEXT("  EXTRA [%s] %s anim=%s postProcess=%s ponytailTipInHead=%s"),
					NetName(W), *GetNameSafe(S->GetSkeletalMeshAsset()), *GetNameSafe(S->GetAnimInstance() ? S->GetAnimInstance()->GetClass() : nullptr),
					*GetNameSafe(S->GetPostProcessInstance() ? S->GetPostProcessInstance()->GetClass() : nullptr), *Tip.ToCompactString());
			}

			TArray<AActor*> Attached;
			C->GetAttachedActors(Attached, false, false);
			for (AActor* A : Attached)
			{
				USceneComponent* Root = A ? A->GetRootComponent() : nullptr;
				if (!Root || A->IsA<APawn>())
				{
					continue;
				}
				USceneComponent* Parent = Root->GetAttachParent();
				const FName Socket = Root->GetAttachSocketName();
				const FVector SocketWorld = Parent ? Parent->GetSocketLocation(Socket) : FVector::ZeroVector;
				// The weapon mesh's own sockets (hit-trace points) and their distance from the visible body.
				FString Points;
				if (UStaticMeshComponent* SM = A->FindComponentByClass<UStaticMeshComponent>())
				{
					for (const FName& N : SM->GetAllSocketNames())
					{
						Points += FString::Printf(TEXT("%s@%.0f "), *N.ToString(), BodyMesh ? FVector::Dist(SM->GetSocketLocation(N), BodyMesh->GetBoneLocation(TEXT("hand_r"))) : -1.f);
					}
				}
				bool bVisible = !A->IsHidden();
				TArray<UPrimitiveComponent*> Prims;
				A->GetComponents<UPrimitiveComponent>(Prims);
				for (UPrimitiveComponent* P : Prims)
				{
					bVisible = bVisible && (P->IsVisible() && !P->bHiddenInGame);
				}
				UE_LOG(LogGOTABodyTest, Log, TEXT("  WEAPON [%s] %s parent=%s socket=%s onBody=%d visible=%d rootToSocket=%.1f traceSockets(dist to visible hand_r)=[%s]"),
					NetName(W), *A->GetName(), *GetNameSafe(Parent), *Socket.ToString(), Parent && Parent == BodyMesh ? 1 : 0, bVisible ? 1 : 0,
					FVector::Dist(Root->GetComponentLocation(), SocketWorld), *Points);
			}
		}
	}

	static FDelegateHandle SpawnHandle;
	static TWeakObjectPtr<UWorld> SpawnWorld;

	static void OnSpawned(AActor* A)
	{
		if (!A || A->IsA<APawn>() || A->IsA<AController>())
		{
			return;
		}
		APawn* By = A->GetInstigator();
		if (!By)
		{
			By = Cast<APawn>(A->GetOwner());
		}
		ACharacter* C = Cast<ACharacter>(By);
		if (!C)
		{
			return;
		}
		UGOTAHeroBodyComponent* Comp = C->FindComponentByClass<UGOTAHeroBodyComponent>();
		USkeletalMeshComponent* BodyMesh = Comp ? Comp->GetBodyMesh() : nullptr;
		USkeletalMeshComponent* Gameplay = C->GetMesh();
		const FVector L = A->GetActorLocation();
		auto D = [&L](USkeletalMeshComponent* M, const TCHAR* Bone) { return M && M->GetBoneIndex(Bone) != INDEX_NONE ? FVector::Dist(L, M->GetBoneLocation(Bone)) : -1.f; };
		UE_LOG(LogGOTABodyTest, Log, TEXT("SPAWN [%s] %s by %s at %s  dist to visible hand_l=%.1f hand_r=%.1f | gameplay hand_l=%.1f"),
			NetName(A->GetWorld()), *A->GetClass()->GetName(), *C->GetName(), *L.ToCompactString(),
			D(BodyMesh, TEXT("hand_l")), D(BodyMesh, TEXT("hand_r")), D(Gameplay, TEXT("hand_l")));
	}

	/** gota.Test.WatchSpawns [0]: log every actor a character spawns (spells, projectiles, effects actors). */
	static void WatchSpawns(const TArray<FString>& Args)
	{
		if (SpawnWorld.IsValid())
		{
			SpawnWorld->RemoveOnActorSpawnedHandler(SpawnHandle);
			SpawnWorld.Reset();
		}
		UWorld* W = GameWorld();
		if (W && !(Args.Num() > 0 && Args[0] == TEXT("0")))
		{
			SpawnHandle = W->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateStatic(&OnSpawned));
			SpawnWorld = W;
			UE_LOG(LogGOTABodyTest, Log, TEXT("WatchSpawns on [%s]"), NetName(W));
		}
	}

	/** gota.Test.BodyScale <scale>: rescale every visible hero body on this machine (tuning only), then log gaps. */
	static void BodyScale(const TArray<FString>& Args)
	{
		UWorld* W = GameWorld();
		const float S = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 1.f;
		if (!W || S <= 0.f)
		{
			return;
		}
		for (TActorIterator<ACharacter> It(W); It; ++It)
		{
			UGOTAHeroBodyComponent* Comp = It->FindComponentByClass<UGOTAHeroBodyComponent>();
			if (USkeletalMeshComponent* BodyMesh = Comp ? Comp->GetBodyMesh() : nullptr)
			{
				BodyMesh->SetRelativeScale3D(FVector(S));
			}
		}
		UE_LOG(LogGOTABodyTest, Log, TEXT("BodyScale %.2f"), S);
	}
	static FAutoConsoleCommand CmdBodyScale(TEXT("gota.Test.BodyScale"), TEXT("Rescale visible hero bodies (tuning)."), FConsoleCommandWithArgsDelegate::CreateStatic(&BodyScale));

	/** gota.Test.LookAt <ClassSubstring> [dist=260] [yaw=30] [n=0]: view the n-th matching character from the front (0 args = player view). */
	static TWeakObjectPtr<ACameraActor> GLookCam;
	static void LookAt(const TArray<FString>& Args)
	{
		UWorld* W = GameWorld();
		APlayerController* PC = W ? W->GetFirstPlayerController() : nullptr;
		if (!PC)
		{
			return;
		}
		if (Args.Num() == 0)
		{
			PC->SetViewTarget(PC->GetPawn());
			return;
		}
		const float Dist = Args.Num() > 1 ? FCString::Atof(*Args[1]) : 260.f;
		const float Yaw = Args.Num() > 2 ? FCString::Atof(*Args[2]) : 30.f;
		int32 Skip = Args.Num() > 3 ? FCString::Atoi(*Args[3]) : 0;
		for (TActorIterator<ACharacter> It(W); It; ++It)
		{
			if (!It->GetClass()->GetName().Contains(Args[0]) || Skip-- > 0)
			{
				continue;
			}
			const FVector Target = It->GetActorLocation() + FVector(0, 0, 40);
			const FRotator Facing(0.f, It->GetActorRotation().Yaw + 180.f - Yaw, 0.f);
			const FVector CamLoc = Target - Facing.Vector() * Dist + FVector(0, 0, 30);
			if (!GLookCam.IsValid())
			{
				GLookCam = W->SpawnActor<ACameraActor>(CamLoc, FRotator::ZeroRotator);
			}
			GLookCam->SetActorLocation(CamLoc);
			GLookCam->SetActorRotation((Target - CamLoc).Rotation());
			PC->SetViewTarget(GLookCam.Get());
			UE_LOG(LogGOTABodyTest, Log, TEXT("LookAt %s at %s"), *It->GetName(), *It->GetActorLocation().ToCompactString());
			return;
		}
	}
	static FAutoConsoleCommand CmdLookAt(TEXT("gota.Test.LookAt"), TEXT("Camera on a character: <ClassSubstring> [dist] [yaw] [n]."), FConsoleCommandWithArgsDelegate::CreateStatic(&LookAt));

/**
 * gota.Test.RebuildBodies <seconds> [interval=0.25]: on this machine, destroy and rebuild every hero body repeatedly
 * (a new skeletal mesh component each time, as on spawn and respawn). Stresses the first frames of cloth meshes.
 */
static FTSTicker::FDelegateHandle GRebuildTicker;
static void RebuildBodies(const TArray<FString>& Args)
{
	const double Until = FPlatformTime::Seconds() + (Args.Num() > 0 ? FCString::Atof(*Args[0]) : 30.0);
	const float Interval = Args.Num() > 1 ? FMath::Max(0.02f, FCString::Atof(*Args[1])) : 0.25f;
	TSharedRef<int32> Count = MakeShared<int32>(0);
	FTSTicker::GetCoreTicker().RemoveTicker(GRebuildTicker);
	GRebuildTicker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Until, Count](float)
	{
		UWorld* World = GOTABodyTest::GameWorld();
		if (!World || FPlatformTime::Seconds() > Until)
		{
			UE_LOG(LogGOTABodyTest, Log, TEXT("BODY rebuild finished: %d rebuilds"), *Count);
			return false;
		}
		for (TActorIterator<ACharacter> It(World); It; ++It)
		{
			if (UGOTAHeroBodyComponent* Body = It->FindComponentByClass<UGOTAHeroBodyComponent>())
			{
				Body->DebugRebuildBody();
				++*Count;
			}
		}
		return true;
	}), Interval);
	UE_LOG(LogGOTABodyTest, Log, TEXT("BODY rebuild every %.2fs"), Interval);
}
/**
 * gota.Test.SceneWatch <seconds> [interval=5]: on this machine, log every skeletal mesh component on every character:
 * owner, being destroyed, registered, visible, cloth (suspended or not), active montage, time since last render.
 * Renderer crash isolation (which meshes outlive a Horde victory on a client).
 */
static FTSTicker::FDelegateHandle GSceneWatchTicker;
static void SceneWatch(const TArray<FString>& Args)
{
	const double Until = FPlatformTime::Seconds() + (Args.Num() > 0 ? FCString::Atof(*Args[0]) : 300.0);
	const float Interval = Args.Num() > 1 ? FMath::Max(0.5f, FCString::Atof(*Args[1])) : 5.f;
	FTSTicker::GetCoreTicker().RemoveTicker(GSceneWatchTicker);
	GSceneWatchTicker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Until](float)
	{
		UWorld* World = GOTABodyTest::GameWorld();
		if (!World || FPlatformTime::Seconds() > Until)
		{
			UE_LOG(LogGOTABodyTest, Log, TEXT("SCENE watch finished"));
			return false;
		}
		int32 Count = 0;
		for (TActorIterator<ACharacter> It(World); It; ++It)
		{
			TInlineComponentArray<USkeletalMeshComponent*> Meshes(*It);
			for (USkeletalMeshComponent* M : Meshes)
			{
				if (!M || !M->IsRegistered() || !M->IsVisible() || M->bHiddenInGame)
				{
					continue;
				}
				++Count;
				const UAnimInstance* Anim = M->GetAnimInstance();
				const UAnimMontage* Montage = Anim ? Anim->GetCurrentActiveMontage() : nullptr;
				const bool bCloth = M->GetSkeletalMeshAsset() && M->GetSkeletalMeshAsset()->HasActiveClothingAssets();
				if (bCloth || It->IsActorBeingDestroyed() || !IsValid(*It) || It->GetLocalRole() == ROLE_SimulatedProxy && It->GetName().Contains(TEXT("Boss")))
				{
					UE_LOG(LogGOTABodyTest, Log, TEXT("SCENE %s.%s mesh=%s cloth=%d clothSuspended=%d destroying=%d tornOff=%d montage=%s sinceRender=%.1fs"),
						*It->GetName(), *M->GetName(), *GetNameSafe(M->GetSkeletalMeshAsset()), bCloth ? 1 : 0, M->IsClothingSimulationSuspended() ? 1 : 0,
						It->IsActorBeingDestroyed() ? 1 : 0, It->GetTearOff() ? 1 : 0, *GetNameSafe(Montage), World->GetTimeSeconds() - M->GetLastRenderTime());
				}
			}
		}
		UE_LOG(LogGOTABodyTest, Log, TEXT("SCENE visible skeletal meshes=%d"), Count);
		return true;
	}), Interval);
	UE_LOG(LogGOTABodyTest, Log, TEXT("SCENE watch every %.1fs"), Interval);
}
/**
 * gota.Test.BodyDrift <seconds>: how far each living hero body's pelvis strays horizontally from its gameplay capsule.
 * The capsule is what the server moves and hits; a body that drifts from it shows attacks and dodges where they are not.
 * Logs, per character with a body: samples, largest and mean distance, and the montage playing at the largest.
 */
static void BodyDrift(const TArray<FString>& Args)
{
	struct FDrift { float Max = 0.f; double Sum = 0.0; int32 Samples = 0; FString AtMax; };
	static TMap<TWeakObjectPtr<ACharacter>, FDrift> Drift;
	static FTSTicker::FDelegateHandle Handle;
	const float Seconds = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 10.f;
	if (Handle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(Handle);
	}
	Drift.Reset();
	const double End = FPlatformTime::Seconds() + Seconds;
	Handle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([End](float)
	{
		UWorld* World = GameWorld();
		const bool bDone = !World || FPlatformTime::Seconds() >= End;
		if (World && !bDone)
		{
			for (TActorIterator<ACharacter> It(World); It; ++It)
			{
				const UGOTAHeroBodyComponent* BodyComp = It->FindComponentByClass<UGOTAHeroBodyComponent>();
				const USkeletalMeshComponent* Body = BodyComp ? BodyComp->GetBodyMesh() : nullptr;
				// Living fighters only: once defeated the capsule no longer fights, and a death fall may travel.
				if (!Body || Body->GetBoneIndex(TEXT("pelvis")) == INDEX_NONE || UGOTACombatantObserver::IsCombatantDefeated(*It))
				{
					continue;
				}
				const float D = FVector::Dist2D(Body->GetBoneLocation(TEXT("pelvis")), It->GetActorLocation());
				FDrift& Entry = Drift.FindOrAdd(*It);
				Entry.Sum += D;
				++Entry.Samples;
				if (D > Entry.Max)
				{
					Entry.Max = D;
					const UAnimInstance* Source = It->GetMesh() ? It->GetMesh()->GetAnimInstance() : nullptr;
					const UAnimMontage* M = Source ? Source->GetCurrentActiveMontage() : nullptr;
					const UCharacterMovementComponent* Move = It->GetCharacterMovement();
					Entry.AtMax = FString::Printf(TEXT("%s speed=%.0f%s"), *GetNameSafe(M), It->GetVelocity().Size2D(), Move && Move->IsFalling() ? TEXT(" air") : TEXT(""));
				}
			}
			return true;
		}
		for (const TPair<TWeakObjectPtr<ACharacter>, FDrift>& P : Drift)
		{
			const UGOTAHeroBodyComponent* BodyComp = P.Key.IsValid() ? P.Key->FindComponentByClass<UGOTAHeroBodyComponent>() : nullptr;
			UE_LOG(LogGOTABodyTest, Log, TEXT("BODYDRIFT %s hero=%s samples=%d max=%.1f mean=%.1f at %s"), *GetNameSafe(P.Key.Get()),
				BodyComp ? *BodyComp->GetHeroId().ToString() : TEXT("?"), P.Value.Samples, P.Value.Max,
				P.Value.Samples ? P.Value.Sum / P.Value.Samples : 0.0, *P.Value.AtMax);
		}
		Handle.Reset();
		return false;
	}));
	UE_LOG(LogGOTABodyTest, Log, TEXT("BODYDRIFT watching %.0fs"), Seconds);
}
/**
 * gota.Test.BodyWatch <seconds>: on this machine, log every change of what each hero body shows: the hidden combat
 * frame's montage and, for a body with its own clips, the clip chosen for it (or the retargeted fallback).
 */
static void BodyWatch(const TArray<FString>& Args)
{
	static TMap<TWeakObjectPtr<ACharacter>, FString> Last;
	static FTSTicker::FDelegateHandle Handle;
	const float Seconds = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 10.f;
	if (Handle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(Handle);
	}
	Last.Reset();
	const double End = FPlatformTime::Seconds() + Seconds;
	Handle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([End](float)
	{
		UWorld* World = GameWorld();
		if (!World || FPlatformTime::Seconds() >= End)
		{
			Handle.Reset();
			return false;
		}
		for (TActorIterator<ACharacter> It(World); It; ++It)
		{
			const UGOTAHeroBodyComponent* BodyComp = It->FindComponentByClass<UGOTAHeroBodyComponent>();
			const USkeletalMeshComponent* Body = BodyComp ? BodyComp->GetBodyMesh() : nullptr;
			if (!Body)
			{
				continue;
			}
			const UAnimInstance* Source = It->GetMesh() ? It->GetMesh()->GetAnimInstance() : nullptr;
			const UAnimMontage* M = Source ? Source->GetCurrentActiveMontage() : nullptr;
			FString Shows = TEXT("-");
			if (const UGOTARetargetAnimInstance* Layer = Cast<UGOTARetargetAnimInstance>(Body->GetAnimInstance()))
			{
				const FGOTABodyLayerFrame F = Layer->GetLayerFrame();
				Shows = !F.bActive ? TEXT("retarget") : F.Action ? GetNameSafe(F.Action) : F.RetargetWeight > 0.5f ? TEXT("fallback") : TEXT("own locomotion");
			}
			const FString Now = FString::Printf(TEXT("montage=%s body=%s"), *GetNameSafe(M), *Shows);
			FString& Prev = Last.FindOrAdd(*It);
			if (Prev != Now)
			{
				Prev = Now;
				UE_LOG(LogGOTABodyTest, Log, TEXT("BODYWATCH [%s] %s (%s) %s"), World->GetNetMode() == NM_Client ? TEXT("client") : TEXT("server"),
					*It->GetName(), It->IsLocallyControlled() ? TEXT("local") : TEXT("remote"), *Now);
			}
		}
		return true;
	}));
	UE_LOG(LogGOTABodyTest, Log, TEXT("BODYWATCH watching %.0fs"), Seconds);
}
FAutoConsoleCommand CmdBodyWatch(TEXT("gota.Test.BodyWatch"), TEXT("Log each change of what hero bodies show (frame montage, own clip): <seconds>."), FConsoleCommandWithArgsDelegate::CreateStatic(&BodyWatch));
FAutoConsoleCommand CmdBodyDrift(TEXT("gota.Test.BodyDrift"), TEXT("Largest horizontal distance of each hero body's pelvis from its capsule: <seconds>."), FConsoleCommandWithArgsDelegate::CreateStatic(&BodyDrift));
FAutoConsoleCommand CmdSceneWatch(TEXT("gota.Test.SceneWatch"), TEXT("Log visible skeletal meshes per character (cloth, destroyed, montage): <seconds> [interval]."), FConsoleCommandWithArgsDelegate::CreateStatic(&SceneWatch));
FAutoConsoleCommand CmdRebuildBodies(TEXT("gota.Test.RebuildBodies"), TEXT("Rebuild every hero body repeatedly on this machine: <seconds> [interval]."), FConsoleCommandWithArgsDelegate::CreateStatic(&RebuildBodies));
FAutoConsoleCommand CmdBody(TEXT("gota.Test.Body"), TEXT("Log hero bodies, hand gaps and kept weapons on this machine."), FConsoleCommandWithArgsDelegate::CreateStatic(&Body));
	static FAutoConsoleCommand CmdSpawns(TEXT("gota.Test.WatchSpawns"), TEXT("Log actors spawned by characters (0 = stop)."), FConsoleCommandWithArgsDelegate::CreateStatic(&WatchSpawns));
}

#endif // !UE_BUILD_SHIPPING
