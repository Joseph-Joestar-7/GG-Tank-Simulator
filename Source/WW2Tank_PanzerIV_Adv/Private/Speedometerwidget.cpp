#include "SpeedometerWidget.h"
#include "Rendering/DrawElements.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"

USpeedometerWidget::USpeedometerWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SpeedFont = FCoreStyle::GetDefaultFontStyle("Bold", 40);
	UnitFont = FCoreStyle::GetDefaultFontStyle("Bold", 16);
}

void USpeedometerWidget::SetSpeed(float NewSpeed)
{
	TargetSpeed = FMath::Max(0.f, NewSpeed);
}

void USpeedometerWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	DisplayedSpeed = (InterpSpeed <= 0.f)
		? TargetSpeed
		: FMath::FInterpTo(DisplayedSpeed, TargetSpeed, InDeltaTime, InterpSpeed);
}

static FVector2f PointOnCircle(const FVector2f& Center, float Radius, float DegFromTop)
{
	const float Rad = FMath::DegreesToRadians(DegFromTop);
	return FVector2f(Center.X + Radius * FMath::Sin(Rad), Center.Y - Radius * FMath::Cos(Rad));
}

void USpeedometerWidget::DrawArc(FSlateWindowElementList& Out, int32 Layer, const FGeometry& Geo,
	const FVector2f& Center, float Radius, float FromDeg, float ToDeg, float Thickness,
	const FLinearColor& Color) const
{
	if (FMath::Abs(ToDeg - FromDeg) < KINDA_SMALL_NUMBER)
	{
		return;
	}
	const int32 Segments = FMath::Max(2, FMath::CeilToInt(FMath::Abs(ToDeg - FromDeg) / 3.f));
	TArray<FVector2f> Points;
	Points.Reserve(Segments + 1);
	for (int32 i = 0; i <= Segments; ++i)
	{
		const float Deg = FMath::Lerp(FromDeg, ToDeg, static_cast<float>(i) / Segments);
		Points.Add(PointOnCircle(Center, Radius, Deg));
	}
	FSlateDrawElement::MakeLines(Out, Layer, Geo.ToPaintGeometry(), Points,
		ESlateDrawEffect::None, Color, true, Thickness);
}

void USpeedometerWidget::DrawCenteredText(FSlateWindowElementList& Out, int32 Layer, const FGeometry& Geo,
	const FString& Text, const FSlateFontInfo& Font, const FVector2f& Center, const FLinearColor& Color) const
{
	const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const FVector2f Size = FVector2f(Measure->Measure(Text, Font));
	FSlateDrawElement::MakeText(Out, Layer,
		Geo.ToOffsetPaintGeometry(Center - Size * 0.5f),
		Text, Font, ESlateDrawEffect::None, Color);
}

int32 USpeedometerWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	LayerId = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId,
		InWidgetStyle, bParentEnabled);

	const FVector2f Size = AllottedGeometry.GetLocalSize();
	const FVector2f Center = Size * 0.5f;
	const float R = FMath::Min(Size.X, Size.Y) * 0.5f;
	if (R < 4.f)
	{
		return LayerId;
	}

	const float Alpha = InWidgetStyle.GetColorAndOpacityTint().A;
	auto Tint = [Alpha](FLinearColor C) { C.A *= Alpha; return C; };

	const float EndAngle = StartAngle + SweepAngle;
	const float Fill = (MaxSpeed > 0.f) ? FMath::Clamp(DisplayedSpeed / MaxSpeed, 0.f, 1.f) : 0.f;
	const float FillAngle = StartAngle + SweepAngle * Fill;

	// --- Dark center disc (thick circle trick: ring of radius r/2 with thickness r) ---
	const float DiscR = R * 0.62f;
	DrawArc(OutDrawElements, LayerId, AllottedGeometry, Center, DiscR * 0.5f, 0.f, 360.f, DiscR, Tint(CenterColor));
	++LayerId;

	// --- Track (background arc) ---
	const float ArcR = R * 0.74f;
	const float ArcThickness = R * 0.075f;
	DrawArc(OutDrawElements, LayerId, AllottedGeometry, Center, ArcR, StartAngle, EndAngle, ArcThickness, Tint(TrackColor));
	++LayerId;

	// --- Glow + filled arc ---
	if (Fill > 0.f)
	{
		for (int32 g = 3; g >= 1; --g)
		{
			FLinearColor Glow = ArcColor;
			Glow.A = 0.12f;
			DrawArc(OutDrawElements, LayerId, AllottedGeometry, Center, ArcR, StartAngle, FillAngle,
				ArcThickness + g * R * 0.035f, Tint(Glow));
		}
		++LayerId;
		DrawArc(OutDrawElements, LayerId, AllottedGeometry, Center, ArcR, StartAngle, FillAngle, ArcThickness, Tint(ArcColor));
		++LayerId;

		// Bright cap at the tip
		const FVector2f Tip = PointOnCircle(Center, ArcR, FillAngle);
		const FVector2f TipIn = PointOnCircle(Center, ArcR - ArcThickness * 0.9f, FillAngle);
		const FVector2f TipOut = PointOnCircle(Center, ArcR + ArcThickness * 0.9f, FillAngle);
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
			TArray<FVector2f>({ TipIn, TipOut }), ESlateDrawEffect::None, Tint(FLinearColor::White), true, 2.f);
		++LayerId;
	}

	// --- Thin ring around the center disc ---
	DrawArc(OutDrawElements, LayerId, AllottedGeometry, Center, DiscR, 0.f, 360.f, 1.5f, Tint(ArcColor * 0.8f));
	++LayerId;

	// --- Outer tick marks ---
	const int32 Ticks = FMath::Max(2, NumTicks);
	for (int32 i = 0; i <= Ticks; ++i)
	{
		const float T = static_cast<float>(i) / Ticks;
		const float Deg = StartAngle + SweepAngle * T;
		const bool bMajor = (i % 5 == 0);
		const float InnerR = R * (bMajor ? 0.86f : 0.90f);
		const float OuterR = R * 0.97f;
		const bool bLit = T <= Fill;
		FLinearColor C = bLit ? FLinearColor::LerpUsingHSV(TickColor, ArcColor, 0.5f) : TickColor;
		C.A = bLit ? 1.f : TickColor.A * 0.7f;
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
			TArray<FVector2f>({ PointOnCircle(Center, InnerR, Deg), PointOnCircle(Center, OuterR, Deg) }),
			ESlateDrawEffect::None, Tint(C), true, bMajor ? 2.f : 1.2f);
	}
	++LayerId;

	// --- Texts ---
	const FString SpeedStr = FString::FromInt(FMath::RoundToInt(DisplayedSpeed));
	DrawCenteredText(OutDrawElements, LayerId, AllottedGeometry, SpeedStr, SpeedFont,
		Center + FVector2f(0.f, -R * 0.08f), Tint(TextColor));
	DrawCenteredText(OutDrawElements, LayerId, AllottedGeometry, UnitText.ToString(), UnitFont,
		Center + FVector2f(0.f, R * 0.3f), Tint(TextColor));

	return LayerId;
}