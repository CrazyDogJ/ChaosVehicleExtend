// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Actor.h"
#include "VehicleModuleBaseActor.generated.h"

class UModularVehicleBaseComponent;

USTRUCT(BlueprintType)
struct FVehicleModuleAttachment
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UPrimitiveComponent* PrimitiveComponent = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	EAttachmentRule AttachmentRule = EAttachmentRule::SnapToTarget;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName Socket = "";
};

UCLASS(BlueprintType, Blueprintable)
class CHAOSVEHICLEEXTEND_API AVehicleModuleBaseActor : public AActor, public IAbilitySystemInterface
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Vehicle Module")
	UMeshComponent* RootMeshComponent = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, ReplicatedUsing = OnRep_AttachmentData, Category = "Vehicle Module")
	FVehicleModuleAttachment AttachmentData;

	void SetAttachmentData(const FVehicleModuleAttachment& InAttachmentData);
	
protected:
	
	UFUNCTION()
	void OnRep_AttachmentData();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
};
