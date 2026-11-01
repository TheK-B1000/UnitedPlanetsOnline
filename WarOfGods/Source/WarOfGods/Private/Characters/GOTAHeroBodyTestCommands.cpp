// White Tiger Studios 2024
// Development-only checks for hero bodies: where the visible hands, kept weapons and spawned spells sit
// relative to the hidden gameplay mesh. Runs on whichever machine executes it (server or client).

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "Characters/GOTAHeroBodyComponent.h"
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

	static FAutoConsoleCommand CmdBody(TEXT("gota.Test.Body"), TEXT("Log hero bodies, hand gaps and kept weapons on this machine."), FConsoleCommandWithArgsDelegate::CreateStatic(&Body));
	static FAutoConsoleCommand CmdSpawns(TEXT("gota.Test.WatchSpawns"), TEXT("Log actors spawned by characters (0 = stop)."), FConsoleCommandWithArgsDelegate::CreateStatic(&WatchSpawns));
}

#endif // !UE_BUILD_SHIPPING
