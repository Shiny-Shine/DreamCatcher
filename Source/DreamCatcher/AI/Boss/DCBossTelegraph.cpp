#include "AI/Boss/DCBossTelegraph.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ADCBossTelegraph::ADCBossTelegraph()
{
	PrimaryActorTick.bCanEverTick = false;
	SetReplicates(false);
	Geometry = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("WarningGeometry"));
	RootComponent = Geometry;
	Geometry->SetMobility(EComponentMobility::Movable);
	Geometry->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Geometry->SetGenerateOverlapEvents(false);
	Geometry->SetCanEverAffectNavigation(false);
	Geometry->SetCastShadow(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded()) { Geometry->SetStaticMesh(Cube.Object); }
}

ADCBossTelegraph* ADCBossTelegraph::Make(AActor* Owner, UMaterialInterface* Material, float Seconds)
{
	if (!IsValid(Owner) || !Owner->GetWorld() || Owner->GetWorld()->bIsTearingDown || Owner->GetNetMode() == NM_DedicatedServer) { return nullptr; }
	FActorSpawnParameters Params;
	Params.Owner = Owner;
	Params.ObjectFlags |= RF_Transient;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ADCBossTelegraph* Actor = Owner->GetWorld()->SpawnActor<ADCBossTelegraph>(StaticClass(), FTransform::Identity, Params);
	if (Actor)
	{
		if (Material) { Actor->Geometry->SetMaterial(0, Material); }
		Actor->SetLifeSpan(FMath::Max(Seconds, .05f));
	}
	return Actor;
}

void ADCBossTelegraph::Segment(FVector Start, FVector End, float Width)
{
	const FVector Delta = End - Start;
	if (Delta.IsNearlyZero()) { return; }
	Geometry->AddInstance(FTransform(Delta.Rotation(), (Start + End) * .5,
		FVector(Delta.Length() / 100., Width / 100., Width / 100.)), true);
}

ADCBossTelegraph* ADCBossTelegraph::Line(AActor* Owner, FVector Start, FVector End, UMaterialInterface* Material, float Seconds)
{
	ADCBossTelegraph* Actor = Make(Owner, Material, Seconds);
	if (Actor) { Actor->Segment(Start, End, 4.f); }
	return Actor;
}

ADCBossTelegraph* ADCBossTelegraph::Circle(AActor* Owner, FVector Center, float Radius, UMaterialInterface* Material, float Seconds)
{
	ADCBossTelegraph* Actor = Make(Owner, Material, Seconds);
	if (Actor)
	{
		Center.Z += 4.;
		constexpr int32 Segments = 64;
		for (int32 Index = 0; Index < Segments; ++Index)
		{
			const double A = Index * UE_TWO_PI / Segments;
			const double B = (Index + 1) * UE_TWO_PI / Segments;
			Actor->Segment(Center + FVector(FMath::Cos(A), FMath::Sin(A), 0) * Radius,
				Center + FVector(FMath::Cos(B), FMath::Sin(B), 0) * Radius, 5.f);
		}
	}
	return Actor;
}
