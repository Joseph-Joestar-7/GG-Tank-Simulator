// MinimapWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MinimapWidget.generated.h"

class UImage;
class UOverlay;
class UTextBlock;
class UTexture;
class UTexture2D;
class UMaterialInterface;
class UMaterialInstanceDynamic;

/**
 * Circular, rotating minimap. Builds its own widget tree in C++.
 *
 * Material (Domain = User Interface) must expose:
 *   Texture param : "MapTexture"
 *   Vector  param : "PlayerUV"   (RG = player position in map UV space)
 *   Scalar  param : "Zoom"       (fraction of the map visible across the diameter)
 *   Scalar  param : "Rotation"   (radians, player/camera yaw)
 *
 * Map convention: top-down capture with Pitch -90, Yaw 0 -> image up = world +X, image right = world +Y.
 */
UCLASS(BlueprintType)
class WW2TANK_PANZERIV_ADV_API UMinimapWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// ---- Map setup ----
	/** UI-domain material with the parameters listed above. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Map")
	TObjectPtr<UMaterialInterface> MinimapMaterial;

	/** Render target or static texture of the top-down map. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Map")
	TObjectPtr<UTexture> MapTexture;

	/** World XY of the map's minimum corner (MinX, MinY). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Map")
	FVector2D WorldOrigin = FVector2D::ZeroVector;

	/** World size (unreal units) covered by the whole map texture (square). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Map", meta = (ClampMin = "1"))
	float WorldSize = 100000.f;

	/** Fraction of the whole map visible across the minimap diameter (smaller = more zoomed in). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Map", meta = (ClampMin = "0.01", ClampMax = "1"))
	float Zoom = 0.15f;

	/** true: map rotates, arrow fixed up. false: north up, arrow rotates. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Map")
	bool bRotateWithPlayer = true;

	// ---- Look ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Look")
	float WidgetSize = 220.f;

	/** Optional ring/frame texture drawn above the map. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Look")
	TObjectPtr<UTexture2D> RingTexture;

	/** Optional arrow texture (pointing up). If null a small green square is used. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Look")
	TObjectPtr<UTexture2D> PlayerArrowTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Look")
	FLinearColor PlayerColor = FLinearColor(0.4f, 1.f, 0.6f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Look")
	float PlayerArrowSize = 18.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Look")
	bool bShowCompass = true;

	/** How far inside the widget edge the N/E/S/W letters sit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Look")
	float CompassInset = 8.f;

	// ---- Markers ----
	/** Track an actor. Null icon = tinted diamond. Returns a handle for RemoveMarker. */
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	int32 AddActorMarker(AActor* Target, UTexture2D* Icon, FLinearColor Tint = FLinearColor(1.f, 0.55f, 0.f, 1.f),
		float Size = 14.f, bool bClampToEdge = true);

	/** Fixed world location marker. */
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	int32 AddLocationMarker(FVector WorldLocation, UTexture2D* Icon, FLinearColor Tint = FLinearColor(1.f, 0.55f, 0.f, 1.f),
		float Size = 14.f, bool bClampToEdge = true);

	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void RemoveMarker(int32 Handle);

	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void ClearMarkers();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	struct FMarker
	{
		int32 Handle = 0;
		TWeakObjectPtr<AActor> Actor;
		FVector StaticLocation = FVector::ZeroVector;
		bool bUseActor = false;
		bool bClamp = true;
		bool bDiamond = false;
		UImage* Image = nullptr;
	};

	int32 AddMarkerInternal(AActor* Target, const FVector& Loc, UTexture2D* Icon, const FLinearColor& Tint, float Size, bool bClamp);
	void BuildLayout();

	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> MapMID;
	UPROPERTY(Transient) TObjectPtr<UOverlay> MarkerLayer;
	UPROPERTY(Transient) TObjectPtr<UImage> PlayerArrow;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> CompassLabels; // N, E, S, W

	TArray<FMarker> Markers;
	int32 NextHandle = 1;
};