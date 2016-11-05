// Fill out your copyright notice in the Description page of Project Settings.

#include "FlightSimulatorVR.h"
#include "TargetSphere.h"
#include "Components/DestructibleComponent.h"


// Sets default values
ATargetSphere::ATargetSphere()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	//PrimaryActorTick.bCanEverTick = true;

	Sphere = CreateDefaultSubobject<UDestructibleComponent>(TEXT("Sphere0"));
	Sphere->SetVisibility(false);
	RootComponent = Sphere;

	SelfDestructionDamage = 25000.f;
	SelfDestructionRadius = 5000.f;
	SelfDestructionImpulse = 0.f;

	bExploded = false;
}

void ATargetSphere::Activate()
{
	Sphere->SetVisibility(true);
}

void ATargetSphere::Explode(const FVector & Location)
{
	bExploded = true;

	Sphere->ApplyRadiusDamage(SelfDestructionDamage, Location, SelfDestructionRadius, SelfDestructionImpulse, true);
}
