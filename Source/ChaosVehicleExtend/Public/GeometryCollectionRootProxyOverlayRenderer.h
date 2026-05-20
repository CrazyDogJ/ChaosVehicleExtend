// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GeometryCollection/GeometryCollectionRootProxyRenderer.h"
#include "GeometryCollectionRootProxyOverlayRenderer.generated.h"

UCLASS()
class CHAOSVEHICLEEXTEND_API UGeometryCollectionRootProxyOverlayRenderer : public UObject, public IGeometryCollectionExternalRenderInterface
{
	GENERATED_BODY()

	/** Static mesh components for the root proxy meshes. */
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> StaticMeshComponents;

public:
	//~ Begin IGeometryCollectionExternalRenderInterface Interface.
	virtual void OnRegisterGeometryCollection(UGeometryCollectionComponent& InComponent) override;
	virtual void OnUnregisterGeometryCollection() override;
	virtual void UpdateState(UGeometryCollection const& InGeometryCollection, FTransform const& InComponentTransform, uint32 InStateFlags) override;
	virtual void UpdateRootTransform(UGeometryCollection const& InGeometryCollection, FTransform const& InRootTransform) override;
	virtual void UpdateRootTransforms(UGeometryCollection const& InGeometryCollection, FTransform const& InRootTransform, TArrayView<const FTransform3f> InRootLocalTransforms) override;
	virtual void UpdateTransforms(UGeometryCollection const& InGeometryCollection, TArrayView<const FTransform3f> InTransforms) override;
	virtual bool ShouldUseNativeFallback(uint32 InStateFlags) const override { return (InStateFlags & EState_Broken) != 0; }
	virtual bool CanEverUseNativeFallback() const override { return true; }
	//~ End IGeometryCollectionExternalRenderInterface Interface.

protected:
	/** Current visibility state. */
	bool bIsVisible = true;

private:
	void CreateRootProxyComponents(UGeometryCollectionComponent& InComponent);
	void UpdateRootProxyTransforms(UGeometryCollection const& InGeometryCollection, FTransform const& InRootTransform, TArrayView<const FTransform3f> InLocalRootTransforms);
	void ClearRootProxyComponents();
};
