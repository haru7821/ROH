using UnrealBuildTool;

public class ROH : ModuleRules
{
	public ROH(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// 모듈 루트를 include 경로로 등록 — "Abilities/..." 식 include의 기준 경로
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"AIModule",
			"NavigationSystem",
			"Niagara",
			"UMG",
			"Slate",
			"SlateCore"
		});
	}
}
