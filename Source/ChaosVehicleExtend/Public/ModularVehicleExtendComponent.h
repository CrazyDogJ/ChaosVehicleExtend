// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "VehicleModuleBaseActor.h"
#include "ChaosModularVehicle/ModularVehicleBaseComponent.h"
#include "PhysicsEngine/ClusterUnionComponent.h"
#include "ModularVehicleExtendComponent.generated.h"

USTRUCT(BlueprintType)
struct FSimTreeUpdatesHandle
{
	GENERATED_BODY()

	Chaos::FSimTreeUpdates TreeUpdatesOut;
};

USTRUCT(BlueprintType)
struct FVehicleSeat
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	AActor* Actor;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UPrimitiveComponent* Primitive;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName Socket;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FTransform Offset;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSimComponentCreated, USceneComponent*, SimComp, int, TransformIndex, FSimTreeUpdatesHandle, UpdateHandle);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CHAOSVEHICLEEXTEND_API UModularVehicleExtendComponent : public UModularVehicleBaseComponent
{
	GENERATED_BODY()

public:
	UModularVehicleExtendComponent();
	
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnClusterUnionAddedComponent OnComponentAddedEvent;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnClusterUnionRemovedComponent OnComponentRemovedEvent;
	
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnSimComponentCreated OnSimComponentCreated;

	// Temp removing components.
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "State")
	TArray<UPrimitiveComponent*> RemovingComponents;

	// Temp adding components.
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "State")
	TMap<UPrimitiveComponent*, FVehicleModuleAttachment> CachedAddingComponents;

	// Current vehicle modules.
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Replicated, Category = "State")
	TArray<AVehicleModuleBaseActor*> CurrentModules;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Replicated, Category = "State")
	TArray<FVehicleSeat> VehicleSeats;
	
	UFUNCTION(BlueprintPure, Category = "Modular Vehicle Extend")
	bool IsRemovingComponents() const
	{
		return RemovingComponents.Num() > 0;
	}

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Modular Vehicle Extend")
	void RemoveVehicleModule(AVehicleModuleBaseActor* ModuleActor);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Modular Vehicle Extend")
	void AddVehicleModule(AVehicleModuleBaseActor* ModuleActor, const FVehicleModuleAttachment& AttachmentData);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Modular Vehicle Extend")
	void AddAttachment(const FVehicleSeat& SeatSetting, bool bAttach = true);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Modular Vehicle Extend")
	void RemoveAttachment(AActor* Actor, bool bAttach = true);
	
protected:
	void UpdateAttachment(const float DeltaSecond);

	void UpdateStreamingLevel() const;
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	virtual void CreateAssociatedSimComponents(USceneComponent* ParentComponent, USceneComponent* AttachedComponent, int ParentIndex, int TransformIndex, Chaos::FSimTreeUpdates& TreeUpdatesOut) override;
	virtual void CreateIndependentSimComponents(USceneComponent* RootComponent, USceneComponent* AttachedComponent, int ParentIndex, int TransformIndex, Chaos::FSimTreeUpdates& TreeUpdatesOut) override;

	virtual void AddComponentToSimulationImpl(UPrimitiveComponent* Component, const TArray<FClusterUnionBoneData>& BonesData, const TArray<FClusterUnionBoneData>& RemovedBoneIDs, bool bIsNew) override;
	virtual void RemoveComponentFromSimulationImpl(UPrimitiveComponent* Component, const TArray<FClusterUnionBoneData>& RemovedBonesData) override;
};
