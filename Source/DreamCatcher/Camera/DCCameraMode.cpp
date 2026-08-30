#include "Camera/DCCameraMode.h"

#include "Camera/DCCameraComponent.h"
#include "GameFramework/Pawn.h"

FDCCameraModeView::FDCCameraModeView()
	: Location(FVector::ZeroVector)
	  , Rotation(FRotator::ZeroRotator)
	  , ControlRotation(FRotator::ZeroRotator)
	  , FieldOfView(90.0f)
	  , LookSensitivityMultiplier(1.0f)
{
}

void FDCCameraModeView::Blend(const FDCCameraModeView& Other, float OtherWeight)
{
	if (OtherWeight <= 0.0f)
	{
		return;
	}

	if (OtherWeight >= 1.0f)
	{
		*this = Other;
		return;
	}

	Location = FMath::Lerp(Location, Other.Location, OtherWeight);

	const FRotator RotationDelta = (Other.Rotation - Rotation).GetNormalized();

	Rotation += RotationDelta * OtherWeight;

	const FRotator ControlRotationDelta = (Other.ControlRotation - ControlRotation).GetNormalized();

	ControlRotation += ControlRotationDelta * OtherWeight;

	FieldOfView = FMath::Lerp(FieldOfView, Other.FieldOfView, OtherWeight);

	LookSensitivityMultiplier = FMath::Lerp(LookSensitivityMultiplier, Other.LookSensitivityMultiplier, OtherWeight);
}

UDCCameraMode::UDCCameraMode()
{
	FieldOfView = 90.0f;

	ViewPitchMin = -89.9f;
	ViewPitchMax = 89.9f;

	BlendTime = 0.25f;
	BlendFunction = EDCCameraModeBlendFunction::EaseOut;
	BlendExponent = 4.0f;

	BlendAlpha = 1.0f;
	BlendWeight = 1.0f;
}

UWorld* UDCCameraMode::GetWorld() const
{
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return nullptr;
	}

	const UObject* OuterObject = GetOuter();

	return OuterObject ? OuterObject->GetWorld() : nullptr;
}

UDCCameraComponent*
UDCCameraMode::GetDCCameraComponent() const
{
	return CastChecked<UDCCameraComponent>(GetOuter());
}

AActor* UDCCameraMode::GetTargetActor() const
{
	const UDCCameraComponent* CameraComponent = GetDCCameraComponent();

	return CameraComponent ? CameraComponent->GetTargetActor() : nullptr;
}

FVector UDCCameraMode::GetPivotLocation() const
{
	const AActor* TargetActor = GetTargetActor();

	if (!TargetActor)
	{
		return FVector::ZeroVector;
	}

	if (const APawn* TargetPawn = Cast<APawn>(TargetActor))
	{
		return TargetPawn->GetPawnViewLocation();
	}

	return TargetActor->GetActorLocation();
}

FRotator UDCCameraMode::GetPivotRotation() const
{
	const AActor* TargetActor = GetTargetActor();

	if (!TargetActor)
	{
		return FRotator::ZeroRotator;
	}

	if (const APawn* TargetPawn = Cast<APawn>(TargetActor))
	{
		return TargetPawn->GetViewRotation();
	}

	return TargetActor->GetActorRotation();
}

void UDCCameraMode::UpdateCameraMode(float DeltaTime)
{
	UpdateView(DeltaTime);
	UpdateBlending(DeltaTime);
}

void UDCCameraMode::UpdateView(float DeltaTime)
{
	(void)DeltaTime;

	const FVector PivotLocation = GetPivotLocation();

	FRotator PivotRotation = GetPivotRotation();

	PivotRotation.Pitch = FMath::ClampAngle(PivotRotation.Pitch, ViewPitchMin, ViewPitchMax);

	View.Location = PivotLocation;
	View.Rotation = PivotRotation;
	View.ControlRotation = PivotRotation;
	View.FieldOfView = FieldOfView;
	View.LookSensitivityMultiplier = LookSensitivityMultiplier;
}

void UDCCameraMode::SetBlendWeight(float Weight)
{
	BlendWeight = FMath::Clamp(Weight, 0.0f, 1.0f);

	const float InverseExponent = BlendExponent > 0.0f ? 1.0f / BlendExponent : 1.0f;

	switch (BlendFunction)
	{
	case EDCCameraModeBlendFunction::Linear:
		BlendAlpha = BlendWeight;
		break;

	case EDCCameraModeBlendFunction::EaseIn:
		BlendAlpha = FMath::InterpEaseIn(0.0f, 1.0f, BlendWeight, InverseExponent);
		break;

	case EDCCameraModeBlendFunction::EaseOut:
		BlendAlpha = FMath::InterpEaseOut(0.0f, 1.0f, BlendWeight, InverseExponent);
		break;

	case EDCCameraModeBlendFunction::EaseInOut:
		BlendAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, BlendWeight, InverseExponent);
		break;

	default:
		checkNoEntry();
		break;
	}
}

void UDCCameraMode::UpdateBlending(float DeltaTime)
{
	if (BlendTime > 0.0f)
	{
		BlendAlpha += DeltaTime / BlendTime;
		BlendAlpha = FMath::Min(BlendAlpha, 1.0f);
	}
	else
	{
		BlendAlpha = 1.0f;
	}

	const float Exponent = BlendExponent > 0.0f ? BlendExponent : 1.0f;

	switch (BlendFunction)
	{
	case EDCCameraModeBlendFunction::Linear:
		BlendWeight = BlendAlpha;
		break;

	case EDCCameraModeBlendFunction::EaseIn:
		BlendWeight = FMath::InterpEaseIn(0.0f, 1.0f, BlendAlpha, Exponent);
		break;

	case EDCCameraModeBlendFunction::EaseOut:
		BlendWeight = FMath::InterpEaseOut(0.0f, 1.0f, BlendAlpha, Exponent);
		break;

	case EDCCameraModeBlendFunction::EaseInOut:
		BlendWeight = FMath::InterpEaseInOut(0.0f, 1.0f, BlendAlpha, Exponent);
		break;

	default:
		checkNoEntry();
		break;
	}
}

UDCCameraModeStack::UDCCameraModeStack()
{
	bIsActive = true;
}

void UDCCameraModeStack::ActivateStack()
{
	if (bIsActive)
	{
		return;
	}

	bIsActive = true;

	for (UDCCameraMode* CameraMode : CameraModeStack)
	{
		if (CameraMode)
		{
			CameraMode->OnActivation();
		}
	}
}

void UDCCameraModeStack::DeactivateStack()
{
	if (!bIsActive)
	{
		return;
	}

	bIsActive = false;

	for (UDCCameraMode* CameraMode : CameraModeStack)
	{
		if (CameraMode)
		{
			CameraMode->OnDeactivation();
		}
	}
}

void UDCCameraModeStack::PushCameraMode(TSubclassOf<UDCCameraMode> CameraModeClass)
{
	if (!CameraModeClass)
	{
		return;
	}

	UDCCameraMode* CameraMode = GetCameraModeInstance(CameraModeClass);

	check(CameraMode);

	int32 StackSize = CameraModeStack.Num();

	// 이미 최상단이라면 다시 추가하지 않음.
	if (StackSize > 0 && CameraModeStack[0] == CameraMode)
	{
		return;
	}

	int32 ExistingStackIndex = INDEX_NONE;

	/*
	 * 기존 Stack에서 이 Mode가 실제로 기여하던 비율.
	 *
	 * 같은 Mode를 다시 최상단으로 올릴 때
	 * Blend가 갑자기 0부터 시작하지 않도록 사용.
	 */
	float ExistingStackContribution = 1.0f;

	for (int32 StackIndex = 0; StackIndex < StackSize; ++StackIndex)
	{
		UDCCameraMode* ExistingMode = CameraModeStack[StackIndex];

		if (ExistingMode == CameraMode)
		{
			ExistingStackIndex = StackIndex;

			ExistingStackContribution *= CameraMode->GetBlendWeight();

			break;
		}

		ExistingStackContribution *= 1.0f - ExistingMode->GetBlendWeight();
	}

	if (ExistingStackIndex != INDEX_NONE)
	{
		CameraModeStack.RemoveAt(ExistingStackIndex);

		--StackSize;
	}
	else
	{
		ExistingStackContribution = 0.0f;
	}

	const bool bShouldBlend = CameraMode->GetBlendTime() > 0.0f && StackSize > 0;

	const float InitialBlendWeight = bShouldBlend ? ExistingStackContribution : 1.0f;

	CameraMode->SetBlendWeight(InitialBlendWeight);

	// 0번이 최신 최상단 Mode.
	CameraModeStack.Insert(CameraMode, 0);

	// 가장 아래 Mode는 항상 View 기반이 되어야 함.
	CameraModeStack.Last()->SetBlendWeight(1.0f);

	if (ExistingStackIndex == INDEX_NONE)
	{
		CameraMode->OnActivation();
	}
}

bool UDCCameraModeStack::EvaluateStack(float DeltaTime, FDCCameraModeView& OutCameraModeView)
{
	if (!bIsActive || CameraModeStack.IsEmpty())
	{
		return false;
	}

	UpdateStack(DeltaTime);
	BlendStack(OutCameraModeView);

	return true;
}

UDCCameraMode* UDCCameraModeStack::GetCameraModeInstance(TSubclassOf<UDCCameraMode> CameraModeClass)
{
	check(CameraModeClass);

	for (UDCCameraMode* CameraMode : CameraModeInstances)
	{
		if (CameraMode && CameraMode->GetClass() == CameraModeClass)
		{
			return CameraMode;
		}
	}

	/*
	 * Stack의 Outer는 CameraComponent이므로
	 * CameraMode의 Outer도 CameraComponent가 됨.
	 */
	UDCCameraMode* NewCameraMode = NewObject<UDCCameraMode>(GetOuter(), CameraModeClass);

	check(NewCameraMode);

	CameraModeInstances.Add(NewCameraMode);

	return NewCameraMode;
}

void UDCCameraModeStack::UpdateStack(float DeltaTime)
{
	const int32 StackSize = CameraModeStack.Num();

	if (StackSize <= 0)
	{
		return;
	}

	int32 RemoveIndex = INDEX_NONE;
	int32 RemoveCount = 0;

	for (int32 StackIndex = 0; StackIndex < StackSize; ++StackIndex)
	{
		UDCCameraMode* CameraMode = CameraModeStack[StackIndex];

		check(CameraMode);

		CameraMode->UpdateCameraMode(DeltaTime);

		if (CameraMode->GetBlendWeight() >= 1.0f)
		{
			// 이 Mode가 완전히 적용됐다면 그 아래 Mode들은 더 이상 최종 View에 기여 X.
			RemoveIndex = StackIndex + 1;
			RemoveCount = StackSize - RemoveIndex;
			break;
		}
	}

	if (RemoveCount <= 0)
	{
		return;
	}

	for (int32 StackIndex = RemoveIndex; StackIndex < StackSize; ++StackIndex)
	{
		UDCCameraMode* RemovedMode = CameraModeStack[StackIndex];

		if (RemovedMode)
		{
			RemovedMode->OnDeactivation();
		}
	}

	CameraModeStack.RemoveAt(RemoveIndex, RemoveCount);
}

void UDCCameraModeStack::BlendStack(FDCCameraModeView& OutCameraModeView) const
{
	const int32 StackSize = CameraModeStack.Num();

	if (StackSize <= 0)
	{
		return;
	}

	// 가장 아래 Mode를 Blend 기반으로 사용.
	const UDCCameraMode* BottomMode = CameraModeStack[StackSize - 1];

	check(BottomMode);

	OutCameraModeView = BottomMode->GetCameraModeView();

	// 아래에서 위로 올라가며 새 Mode를 혼합.
	for (int32 StackIndex = StackSize - 2; StackIndex >= 0; --StackIndex)
	{
		const UDCCameraMode* CameraMode = CameraModeStack[StackIndex];

		check(CameraMode);

		OutCameraModeView.Blend(CameraMode->GetCameraModeView(), CameraMode->GetBlendWeight());
	}
}

void UDCCameraModeStack::GetBlendInfo(float& OutTopModeWeight, FGameplayTag& OutTopModeTag) const
{
	if (CameraModeStack.IsEmpty())
	{
		OutTopModeWeight = 0.0f;
		OutTopModeTag = FGameplayTag();
		return;
	}

	// 0번이 현재 최상단 CameraMode.
	const UDCCameraMode* TopMode = CameraModeStack[0];

	check(TopMode);

	OutTopModeWeight = TopMode->GetBlendWeight();

	OutTopModeTag = TopMode->GetCameraTypeTag();
}
