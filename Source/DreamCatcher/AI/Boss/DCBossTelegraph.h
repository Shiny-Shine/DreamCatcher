#pragma once

#include "GameFramework/Actor.h"
#include "DCBossTelegraph.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInterface;

/** R12 standalone presentation only. No damage, collision, navigation or persistent actors. */
UCLASS(NotBlueprintable)
class DREAMCATCHER_API ADCBossTelegraph : public AActor
{
	GENERATED_BODY()
public:
	ADCBossTelegraph();
	static ADCBossTelegraph* Line(AActor* Owner, FVector Start, FVector End, UMaterialInterface* Material, float Seconds);
	static ADCBossTelegraph* Circle(AActor* Owner, FVector Center, float Radius, UMaterialInterface* Material, float Seconds);
private:
	static ADCBossTelegraph* Make(AActor* Owner, UMaterialInterface* Material, float Seconds);
	void Segment(FVector Start, FVector End, float Width);
	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> Geometry;
};
