#include "ModValidationSettings.h"

UModValidationSettings::UModValidationSettings()
{
	// Seed with the known footgun this validator was created for:
	// UFGPowerConnectionComponent::SetPowerInfo(powerInfo) crashes the game when the
	// input is left unconnected (it passes a null/None UFGPowerInfoComponent).
	//
	// Note: this default is only applied on first run / when no entry exists yet in
	// Config/DefaultEditor.ini. Once the list is edited and saved, the .ini wins.
	if (RequiredPins.Num() == 0)
	{
		FRequiredPinRule SetPowerInfoRule;
		SetPowerInfoRule.ClassName = TEXT("FGPowerConnectionComponent");
		SetPowerInfoRule.FunctionName = TEXT("SetPowerInfo");
		SetPowerInfoRule.PinName = TEXT("powerInfo");
		RequiredPins.Add(SetPowerInfoRule);
	}

	// Seed with the hologram/buildable mismatch that crashes on clicking the recipe:
	// AFGRailroadTrackHologram::BeginPlay() does GetDefaultBuildable<AFGBuildableRailroadTrack>()
	// and null-derefs when the buildable isn't actually a railroad track (EXCEPTION_ACCESS_VIOLATION).
	// This rule also enforces that any buildable's mHologramClass is at least an FGHologram.
	if (ClassPropertyRules.Num() == 0)
	{
		FClassPropertyRule HologramRule;
		HologramRule.OwnerClassName = TEXT("FGBuildable");
		HologramRule.PropertyName = TEXT("mHologramClass");
		HologramRule.RequiredBaseClass = TEXT("FGHologram");
		HologramRule.WhenValueDerivesFrom = TEXT("FGRailroadTrackHologram");
		HologramRule.RequiredOwnerBaseClass = TEXT("FGBuildableRailroadTrack");
		ClassPropertyRules.Add(HologramRule);

		// A building descriptor with a null/dangling Buildable Class shows "nullpeter" and crashes
		// the build menu the instant its recipe is selected (renaming a Build_ asset leaves this
		// reference dangling). Fail the package instead of shipping the crash.
		FClassPropertyRule DescBuildableRule;
		DescBuildableRule.OwnerClassName = TEXT("FGBuildingDescriptor");
		DescBuildableRule.PropertyName = TEXT("mBuildableClass");
		DescBuildableRule.bMustBeSet = true;
		ClassPropertyRules.Add(DescBuildableRule);

		// Same crash class for VEHICLES: a UFGVehicleDescriptor with a null mVehicleClass shows
		// "nullpeter" and crashes the build menu the instant its recipe is selected.
		FClassPropertyRule DescVehicleRule;
		DescVehicleRule.OwnerClassName = TEXT("FGVehicleDescriptor");
		DescVehicleRule.PropertyName = TEXT("mVehicleClass");
		DescVehicleRule.bMustBeSet = true;
		ClassPropertyRules.Add(DescVehicleRule);

		// A vehicle is built via its OWN AFGVehicle::mHologramClass. If it's null the build gun
		// spawns no hologram ("no hologram spawned") and null-derefs the instant the recipe is
		// selected. Same crash, different property from the descriptor rule above.
		FClassPropertyRule VehicleHologramRule;
		VehicleHologramRule.OwnerClassName = TEXT("FGVehicle");
		VehicleHologramRule.PropertyName = TEXT("mHologramClass");
		VehicleHologramRule.bMustBeSet = true;
		ClassPropertyRules.Add(VehicleHologramRule);
	}
}
