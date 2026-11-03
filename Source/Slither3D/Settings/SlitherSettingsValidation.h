
#pragma once


struct FSlitherSettingsValidation
{
public:

    static constexpr bool IsInRange(
        const double Value,
        const double Minimum,
        const double Maximum)
    {
        return Value >= Minimum && Value <= Maximum;
    }

    static constexpr bool IsColorNormalized(
        const double Red,
        const double Green,
        const double Blue,
        const double Alpha)
    {
        return Red >= 0.0 && Red <= 1.0
            && Green >= 0.0 && Green <= 1.0
            && Blue >= 0.0 && Blue <= 1.0
            && Alpha == 1.0;
    }

    // Constexpr counterpart of FMath::IsNearlyEqual()
    static constexpr bool IsNearlyEqual(
        const double First,
        const double Second,
        const double Tolerance)
    {
        const double Difference = First - Second;

        const double AbsoluteDifference =
            Difference < 0.0 ? -Difference : Difference;

        return AbsoluteDifference <= Tolerance;
    }
};
