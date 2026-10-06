#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Fonts/SlateFontInfo.h"
#include "SpeedometerWidget.generated.h"

/**
 * Circular speedometer drawn entirely in C++ (no child widgets needed).
 * Add it to your HUD widget blueprint (Palette > User Created > SpeedometerWidget),
 * then call SetSpeed from your HUD / Pawn blueprint.
 *
 */
UCLASS()
class WW2TANK_PANZERIV_ADV_API USpeedometerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	USpeedometerWidget(const FObjectInitializer& ObjectInitializer);

	/** Call this from Blueprint every time your pawn computes the speed. */
	UFUNCTION(BlueprintCallable, Category = "Speedometer")
	void SetSpeed(float NewSpeed);

	UFUNCTION(BlueprintPure, Category = "Speedometer")
	float GetSpeed() const { return TargetSpeed; }

	/** Speed that corresponds to a full gauge. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speedometer")
	float MaxSpeed = 80.f;

	/** Text under the number. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speedometer")
	FText UnitText = FText::FromString(TEXT("KMH"));

	/** How fast the needle/arc catches up with the real value (0 = instant). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speedometer|Style")
	float InterpSpeed = 8.f;

	/** Gauge starts at this angle (degrees, clockwise from 12 o'clock). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speedometer|Style")
	float StartAngle = -135.f;

	/** Total sweep of the gauge in degrees. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speedometer|Style")
	float SweepAngle = 270.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speedometer|Style", meta = (ClampMin = "10", ClampMax = "100"))
	int32 NumTicks = 40;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speedometer|Style")
	FLinearColor ArcColor = FLinearColor(0.f, 0.55f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speedometer|Style")
	FLinearColor TickColor = FLinearColor(0.75f, 0.8f, 0.85f, 0.9f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speedometer|Style")
	FLinearColor TrackColor = FLinearColor(0.05f, 0.15f, 0.3f, 0.8f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speedometer|Style")
	FLinearColor CenterColor = FLinearColor(0.01f, 0.05f, 0.12f, 0.9f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speedometer|Style")
	FLinearColor TextColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speedometer|Style")
	FSlateFontInfo SpeedFont;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speedometer|Style")
	FSlateFontInfo UnitFont;

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	float TargetSpeed = 0.f;
	float DisplayedSpeed = 0.f;

	void DrawArc(FSlateWindowElementList& Out, int32 Layer, const FGeometry& Geo, const FVector2f& Center,
		float Radius, float FromDeg, float ToDeg, float Thickness, const FLinearColor& Color) const;

	void DrawCenteredText(FSlateWindowElementList& Out, int32 Layer, const FGeometry& Geo,
		const FString& Text, const FSlateFontInfo& Font, const FVector2f& Center, const FLinearColor& Color) const;
};