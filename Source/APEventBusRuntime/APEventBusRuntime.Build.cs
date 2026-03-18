// Copyright AfterPrime Systems. All Rights Reserved.

using UnrealBuildTool;

public class APEventBusRuntime : ModuleRules
{
	public APEventBusRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"NetCore",
			"GameplayTags"
		});
	}
}
