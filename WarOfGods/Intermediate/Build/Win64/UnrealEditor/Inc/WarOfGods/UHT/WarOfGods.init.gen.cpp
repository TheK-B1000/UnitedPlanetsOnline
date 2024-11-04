// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeWarOfGods_init() {}
	static FPackageRegistrationInfo Z_Registration_Info_UPackage__Script_WarOfGods;
	FORCENOINLINE UPackage* Z_Construct_UPackage__Script_WarOfGods()
	{
		if (!Z_Registration_Info_UPackage__Script_WarOfGods.OuterSingleton)
		{
			static const UECodeGen_Private::FPackageParams PackageParams = {
				"/Script/WarOfGods",
				nullptr,
				0,
				PKG_CompiledIn | 0x00000000,
				0x88E9454C,
				0x00832943,
				METADATA_PARAMS(0, nullptr)
			};
			UECodeGen_Private::ConstructUPackage(Z_Registration_Info_UPackage__Script_WarOfGods.OuterSingleton, PackageParams);
		}
		return Z_Registration_Info_UPackage__Script_WarOfGods.OuterSingleton;
	}
	static FRegisterCompiledInInfo Z_CompiledInDeferPackage_UPackage__Script_WarOfGods(Z_Construct_UPackage__Script_WarOfGods, TEXT("/Script/WarOfGods"), Z_Registration_Info_UPackage__Script_WarOfGods, CONSTRUCT_RELOAD_VERSION_INFO(FPackageReloadVersionInfo, 0x88E9454C, 0x00832943));
PRAGMA_ENABLE_DEPRECATION_WARNINGS
