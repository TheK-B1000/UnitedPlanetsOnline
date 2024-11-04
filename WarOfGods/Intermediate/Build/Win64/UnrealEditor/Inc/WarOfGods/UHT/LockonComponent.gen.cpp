// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "WarOfGods/Public/LockonComponent.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeLockonComponent() {}
// Cross Module References
	ENGINE_API UClass* Z_Construct_UClass_UActorComponent();
	UPackage* Z_Construct_UPackage__Script_WarOfGods();
	WAROFGODS_API UClass* Z_Construct_UClass_ULockonComponent();
	WAROFGODS_API UClass* Z_Construct_UClass_ULockonComponent_NoRegister();
// End Cross Module References
	void ULockonComponent::StaticRegisterNativesULockonComponent()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(ULockonComponent);
	UClass* Z_Construct_UClass_ULockonComponent_NoRegister()
	{
		return ULockonComponent::StaticClass();
	}
	struct Z_Construct_UClass_ULockonComponent_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_ULockonComponent_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UActorComponent,
		(UObject* (*)())Z_Construct_UPackage__Script_WarOfGods,
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_ULockonComponent_Statics::DependentSingletons) < 16);
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_ULockonComponent_Statics::Class_MetaDataParams[] = {
		{ "BlueprintSpawnableComponent", "" },
		{ "ClassGroupNames", "Custom" },
		{ "IncludePath", "LockonComponent.h" },
		{ "ModuleRelativePath", "Public/LockonComponent.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_ULockonComponent_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<ULockonComponent>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_ULockonComponent_Statics::ClassParams = {
		&ULockonComponent::StaticClass,
		"Engine",
		&StaticCppClassTypeInfo,
		DependentSingletons,
		nullptr,
		nullptr,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		0,
		0,
		0,
		0x00B000A4u,
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_ULockonComponent_Statics::Class_MetaDataParams), Z_Construct_UClass_ULockonComponent_Statics::Class_MetaDataParams)
	};
	UClass* Z_Construct_UClass_ULockonComponent()
	{
		if (!Z_Registration_Info_UClass_ULockonComponent.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_ULockonComponent.OuterSingleton, Z_Construct_UClass_ULockonComponent_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_ULockonComponent.OuterSingleton;
	}
	template<> WAROFGODS_API UClass* StaticClass<ULockonComponent>()
	{
		return ULockonComponent::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(ULockonComponent);
	ULockonComponent::~ULockonComponent() {}
	struct Z_CompiledInDeferFile_FID_UnitedPlanetsOnline_WarOfGods_Source_WarOfGods_Public_LockonComponent_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_UnitedPlanetsOnline_WarOfGods_Source_WarOfGods_Public_LockonComponent_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_ULockonComponent, ULockonComponent::StaticClass, TEXT("ULockonComponent"), &Z_Registration_Info_UClass_ULockonComponent, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(ULockonComponent), 1682811978U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_UnitedPlanetsOnline_WarOfGods_Source_WarOfGods_Public_LockonComponent_h_979863413(TEXT("/Script/WarOfGods"),
		Z_CompiledInDeferFile_FID_UnitedPlanetsOnline_WarOfGods_Source_WarOfGods_Public_LockonComponent_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_UnitedPlanetsOnline_WarOfGods_Source_WarOfGods_Public_LockonComponent_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
