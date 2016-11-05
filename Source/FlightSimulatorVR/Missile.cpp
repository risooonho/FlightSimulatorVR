// Fill out your copyright notice in the Description page of Project Settings.

#include "FlightSimulatorVR.h"
#include "Missile.h"
#include "PhysicsEngine/RadialForceComponent.h"
#include "PhysicsEngine/DestructibleActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DestructibleMesh.h"
#include "Targets.h"
#include "TargetSphere.h"


// Sets default values
AMissile::AMissile()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Structure to hold one-time initialization
	//struct FConstructorStatics
	//{
	//	ConstructorHelpers::FObjectFinderOptional<UDestructibleMesh> Missile;
	//	FConstructorStatics()
	//		: Missile(TEXT("/Game/Geometry/Meshes/AIM120DMissile/AIM120D_DM.AIM120D_DM"))
	//	{
	//	}
	//};
	//static FConstructorStatics ConstructorStatics;

	Missile = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Missile0"));
	Missile->SetCollisionProfileName(TEXT("OverlapAll"));
	RootComponent = Missile;

	Trail = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("Trail0"));
	Trail->SetupAttachment(Missile);

	Explosion = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("Explosion0"));
	Explosion->SetupAttachment(Missile);
	Explosion->bAutoActivate = false;

	RadialForce = CreateDefaultSubobject<URadialForceComponent>(TEXT("RadialForce0"));
	RadialForce->SetupAttachment(Explosion);
	RadialForce->bAutoActivate = false;
	RadialForce->Radius = 2000.f;
	RadialForce->DestructibleDamage = 200.f;

	LockTime = 0.2f;
	FlyTime = 30.f;
	BurnTime = 10.0f;
	InitialSpeed = 50.f;
	MaxSpeed = 6000.f;
	Acceleration = 0.01f;
}

// Called when the game starts or when spawned
void AMissile::BeginPlay()
{
	Super::BeginPlay();

	CurrentForwardSpeed = InitialSpeed;

	CurrentStage = Stage::Created;
}

void AMissile::Activate(float LaunchSpeed, class AActor* MissileLauncher)
{
	if (CurrentStage != Stage::Created)
		return;

	Missile->SetVisibility(true);

	CurrentForwardSpeed += LaunchSpeed;
	CurrentStage = Stage::Activated;

	FTimerHandle UnlockTimerHandle;
	GetWorldTimerManager().SetTimer(UnlockTimerHandle, this, &AMissile::Unlock, LockTime);

	FTimerHandle SelfDestoryTimerHandle;
	GetWorldTimerManager().SetTimer(SelfDestoryTimerHandle, this, &AMissile::Explode, FlyTime);

	Launcher = MissileLauncher;

	//UE_LOG(LogTemp, Warning, TEXT("AMissile::Activate %.4f"), CurrentForwardSpeed);
}

// Called every frame
void AMissile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	//if (rand() % 100 == 0)
	//	UE_LOG(LogTemp, Warning, TEXT("AMissile::Tick [CurrentForwardSpeed = %.4f] [Stage = %d] [GetActorLocation = %s] [GetActorRotation = %s]"), CurrentForwardSpeed, (int32)CurrentStage, *GetActorLocation().ToString(), *GetActorRotation().ToString());

	if (CurrentStage == Stage::Activated || CurrentStage == Stage::Unlocked)
	{
		CurrentForwardSpeed += Acceleration * (MaxSpeed - CurrentForwardSpeed);

		const FVector LocalMove = FVector(CurrentForwardSpeed * DeltaSeconds, 0.f, 0.f);

		// Move plan forwards (with sweep so we stop when we collide with things)
		AddActorLocalOffset(LocalMove, true);

		// Calculate change in rotation this frame
		//FRotator DeltaRotation(0, 0, 0);
		//DeltaRotation.Pitch = CurrentPitchSpeed * DeltaSeconds;
		//DeltaRotation.Yaw = CurrentYawSpeed * DeltaSeconds;
		//DeltaRotation.Roll = CurrentRollSpeed * DeltaSeconds;

		// Rotate plane
		//AddActorLocalRotation(FQuat(DeltaRotation));
	}
}

void AMissile::NotifyHit(class UPrimitiveComponent* MyComp, class AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);

	if (CurrentStage == Stage::Unlocked && Other != Launcher)
	{
		UE_LOG(LogTemp, Warning, TEXT("AMissile::Explode %s"), *Other->GetName());

		// TODO Assume ATargetSphere has Owner
		if (Other->IsA(ATargetSphere::StaticClass()))
		{
			AActor* OtherOwner = Other->GetOwner();
			if (OtherOwner && OtherOwner->IsA(ATargets::StaticClass()))
				Cast<ATargets>(OtherOwner)->OnTargetHit(Other, HitLocation);
		}
			
		Explode();
	}
}

void AMissile::Unlock()
{
	if (CurrentStage != Stage::Activated)
		return;

	Missile->SetCollisionProfileName(TEXT("Destructible"));

	CurrentStage = Stage::Unlocked;
}

void AMissile::Explode()
{
	if (CurrentStage != Stage::Unlocked)
		return;

	Missile->SetCollisionProfileName(TEXT("OverlapAll"));
	Missile->SetVisibility(false);

	Trail->Deactivate();
	Explosion->Activate();
	RadialForce->FireImpulse();

	CurrentStage = Stage::Exploded;
	FTimerHandle BurnTimerHandle;
	GetWorldTimerManager().SetTimer(BurnTimerHandle, this, &AMissile::Remove, BurnTime);
}

void AMissile::Remove()
{
	CurrentStage = Stage::Removed;

	Destroy();
}
