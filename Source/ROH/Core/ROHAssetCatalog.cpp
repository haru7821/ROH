#include "Core/ROHAssetCatalog.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "ROH.h"

namespace
{
	/**
	 * 규약 경로 "/Game/ROH/Art/<키>.<키>"의 애셋 로드 (b32).
	 * 애셋 부재가 정상 상태이므로 LoadObject의 실패 경고(LogUObjectGlobals/LogStreaming)를
	 * 피하기 위해 패키지 존재를 먼저 조용히 확인한다 (에디터/쿡 빌드 공용 경로).
	 */
	template <typename AssetType>
	AssetType* LoadCatalogAsset(FName Key)
	{
		if (Key.IsNone())
		{
			return nullptr;
		}
		const FString KeyString = Key.ToString();
		const FString PackagePath = FString::Printf(TEXT("/Game/ROH/Art/%s"), *KeyString);
		if (!FPackageName::DoesPackageExist(PackagePath))
		{
			return nullptr; // 미임포트 = 그레이박스 폴백 (로그 없음 — 스폰마다 반복되는 정상 경로)
		}

		AssetType* Asset = LoadObject<AssetType>(nullptr, *FString::Printf(TEXT("%s.%s"), *PackagePath, *KeyString));
		if (Asset)
		{
			UE_LOG(LogROH, Verbose, TEXT("애셋 카탈로그: %s 로드 성공"), *KeyString);
		}
		else
		{
			// 패키지는 있는데 로드 실패 = 사용자가 이름은 맞췄지만 타입이 다른 경우가 대부분
			// (예: 스켈레탈 메시 임포트) — docs/13 §4의 "LogROH 검색" 진단이 걸리도록 Warning
			UE_LOG(LogROH, Warning, TEXT("애셋 카탈로그: %s 로드 실패 — 패키지는 존재. 애셋 타입(스태틱 메시/머티리얼)과 애셋 이름이 파일 이름과 같은지 확인"), *KeyString);
		}
		return Asset;
	}
}

UStaticMesh* ROHAssetCatalog::LoadMesh(FName Key)
{
	return LoadCatalogAsset<UStaticMesh>(Key);
}

UMaterialInterface* ROHAssetCatalog::LoadMaterialForMeshKey(FName MeshKey)
{
	// 규약: 메시 키의 "SM_" 접두를 "M_"로 치환 (SM_Waypoint → M_Waypoint)
	const FString MeshKeyString = MeshKey.ToString();
	if (!MeshKeyString.StartsWith(TEXT("SM_")))
	{
		return nullptr;
	}
	return LoadCatalogAsset<UMaterialInterface>(
		FName(*FString::Printf(TEXT("M_%s"), *MeshKeyString.RightChop(3))));
}
