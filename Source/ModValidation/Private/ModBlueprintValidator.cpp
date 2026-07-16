#include "ModBlueprintValidator.h"

#include "ModValidationSettings.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "K2Node_CallFunction.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"
#include "UObject/SoftObjectPtr.h"

#define LOCTEXT_NAMESPACE "ModBlueprintValidator"

UModBlueprintValidator::UModBlueprintValidator()
{
	bIsEnabled = true;
}

bool UModBlueprintValidator::CanValidateAsset_Implementation(
	const FAssetData& InAssetData,
	UObject* InAsset,
	FDataValidationContext& InContext) const
{
	// Only Blueprints have graphs / a generated-class CDO to inspect.
	return Cast<UBlueprint>(InAsset) != nullptr;
}

namespace
{
	/** A pin is "unset" when nothing is wired into it and it carries no usable default. */
	bool IsPinUnset(const UEdGraphPin* Pin)
	{
		return Pin->LinkedTo.Num() == 0
			&& Pin->DefaultObject == nullptr
			&& Pin->DefaultValue.IsEmpty()
			&& Pin->DefaultTextValue.IsEmpty();
	}

	/** Match a rule's class name against a single class, ignoring the U/A prefix. */
	bool ClassNameMatches(const UClass* Class, const FString& RuleName)
	{
		if (!Class)
		{
			return false;
		}
		const FString Name = Class->GetName();
		return Name.Equals(RuleName, ESearchCase::IgnoreCase)
			|| Name.EndsWith(RuleName); // tolerates the "U"/"A" prefix on the rule name
	}

	/** Match a rule's class name against the function's owning class (used by the pin rules). */
	bool ClassMatches(const FString& RuleClassName, const UClass* OwningClass)
	{
		if (RuleClassName.IsEmpty())
		{
			return true; // No class constraint -> match the function on any class.
		}
		return ClassNameMatches(OwningClass, RuleClassName);
	}

	/** True when Class (or any ancestor) matches BaseName. Empty BaseName -> always true. */
	bool ClassIsA(const UClass* Class, const FString& BaseName)
	{
		if (BaseName.IsEmpty())
		{
			return true;
		}
		for (const UClass* Super = Class; Super; Super = Super->GetSuperClass())
		{
			if (ClassNameMatches(Super, BaseName))
			{
				return true;
			}
		}
		return false;
	}

	/** Read a class-valued property (TSubclassOf or soft class) off a CDO. Null if unset / not a class property. */
	UClass* ReadClassProperty(const FProperty* Prop, const UObject* Container)
	{
		if (const FClassProperty* ClassProp = CastField<FClassProperty>(Prop))
		{
			return Cast<UClass>(ClassProp->GetObjectPropertyValue_InContainer(Container));
		}
		if (const FSoftClassProperty* SoftClassProp = CastField<FSoftClassProperty>(Prop))
		{
			FSoftObjectPtr SoftPtr = SoftClassProp->GetPropertyValue_InContainer(Container);
			if (UObject* Resolved = SoftPtr.Get())
			{
				return Cast<UClass>(Resolved);
			}
			return Cast<UClass>(SoftPtr.LoadSynchronous());
		}
		return nullptr;
	}

	/** Collect "required input pin left unconnected" problems. */
	void CollectRequiredPinProblems(const UBlueprint* Blueprint, const UModValidationSettings* Settings, TArray<FString>& Problems)
	{
		if (Settings->RequiredPins.Num() == 0)
		{
			return;
		}

		// Every graph in the Blueprint: event graphs, function graphs (incl. the construction
		// script), macro graphs, etc.
		TArray<UEdGraph*> Graphs;
		Blueprint->GetAllGraphs(Graphs);

		for (const UEdGraph* Graph : Graphs)
		{
			for (const UEdGraphNode* Node : Graph->Nodes)
			{
				const UK2Node_CallFunction* CallNode = Cast<UK2Node_CallFunction>(Node);
				if (!CallNode)
				{
					continue;
				}

				const UFunction* TargetFunction = CallNode->GetTargetFunction();
				const FString FunctionName = TargetFunction
					? TargetFunction->GetName()
					: CallNode->FunctionReference.GetMemberName().ToString();
				const UClass* OwningClass = TargetFunction ? TargetFunction->GetOuterUClass() : nullptr;

				for (const FRequiredPinRule& Rule : Settings->RequiredPins)
				{
					if (!Rule.FunctionName.Equals(FunctionName, ESearchCase::IgnoreCase))
					{
						continue;
					}
					if (!ClassMatches(Rule.ClassName, OwningClass))
					{
						continue;
					}

					const FName RulePinName(*Rule.PinName);
					for (const UEdGraphPin* Pin : CallNode->Pins)
					{
						if (Pin->Direction != EGPD_Input || Pin->PinName != RulePinName)
						{
							continue;
						}
						if (IsPinUnset(Pin))
						{
							Problems.Add(FString::Printf(
								TEXT("'%s.%s' is unconnected in graph '%s'"),
								*FunctionName, *Rule.PinName, *Graph->GetName()));
						}
					}
				}
			}
		}
	}

	/** Collect "class-valued property attaches the wrong type" problems (e.g. wrong hologram). */
	void CollectClassPropertyProblems(const UBlueprint* Blueprint, const UModValidationSettings* Settings, TArray<FString>& Problems)
	{
		if (Settings->ClassPropertyRules.Num() == 0)
		{
			return;
		}

		UClass* GenClass = Blueprint->GeneratedClass;
		if (!GenClass)
		{
			return; // not compiled yet; nothing to read
		}
		const UObject* CDO = GenClass->GetDefaultObject();
		if (!CDO)
		{
			return;
		}

		for (const FClassPropertyRule& Rule : Settings->ClassPropertyRules)
		{
			if (Rule.PropertyName.IsEmpty())
			{
				continue;
			}
			if (!ClassIsA(GenClass, Rule.OwnerClassName))
			{
				continue; // rule doesn't apply to this asset's class
			}

			// FindPropertyByName walks up into native parents, so inherited props like
			// AFGBuildable::mHologramClass are found on a Blueprint's generated class.
			const FProperty* Prop = GenClass->FindPropertyByName(FName(*Rule.PropertyName));
			if (!Prop)
			{
				continue; // property not present on this class
			}

			UClass* Referenced = ReadClassProperty(Prop, CDO);
			if (!Referenced)
			{
				continue; // unset / None -> not this rule's concern
			}

			// 1. Type check: the attached class must be of the required base type.
			if (!Rule.RequiredBaseClass.IsEmpty() && !ClassIsA(Referenced, Rule.RequiredBaseClass))
			{
				Problems.Add(FString::Printf(
					TEXT("'%s' is set to '%s', which is not a '%s'"),
					*Rule.PropertyName, *Referenced->GetName(), *Rule.RequiredBaseClass));
			}

			// 2. Pairing check: an attached class of a given family requires a matching owner family.
			const bool bTriggered = Rule.WhenValueDerivesFrom.IsEmpty() || ClassIsA(Referenced, Rule.WhenValueDerivesFrom);
			if (bTriggered && !Rule.RequiredOwnerBaseClass.IsEmpty() && !ClassIsA(GenClass, Rule.RequiredOwnerBaseClass))
			{
				Problems.Add(FString::Printf(
					TEXT("'%s' is a '%s' but this asset ('%s') is not a '%s' - that class mismatch crashes the game on use"),
					*Rule.PropertyName, *Referenced->GetName(), *GenClass->GetName(), *Rule.RequiredOwnerBaseClass));
			}
		}
	}
}

EDataValidationResult UModBlueprintValidator::ValidateLoadedAsset_Implementation(
	const FAssetData& InAssetData,
	UObject* InAsset,
	FDataValidationContext& InContext)
{
	UBlueprint* Blueprint = Cast<UBlueprint>(InAsset);
	const UModValidationSettings* Settings = GetDefault<UModValidationSettings>();

	if (!Blueprint || !Settings)
	{
		AssetPasses(InAsset);
		return EDataValidationResult::Valid;
	}

	TArray<FString> Problems;
	CollectRequiredPinProblems(Blueprint, Settings, Problems);
	CollectClassPropertyProblems(Blueprint, Settings, Problems);

	if (Problems.Num() > 0)
	{
		AssetFails(InAsset, FText::Format(
			LOCTEXT("ValidationProblems",
				"{0} validation problem(s) found; a null/mismatched value here can crash the game:\n  - {1}"),
			FText::AsNumber(Problems.Num()),
			FText::FromString(FString::Join(Problems, TEXT("\n  - ")))));
		return EDataValidationResult::Invalid;
	}

	AssetPasses(InAsset);
	return EDataValidationResult::Valid;
}

#undef LOCTEXT_NAMESPACE
