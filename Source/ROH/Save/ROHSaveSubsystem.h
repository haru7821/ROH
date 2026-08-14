#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ROHSaveSubsystem.generated.h"

class AROHPlayerCharacter;

/**
 * 로컬 세이브/로드 (docs/03 §3.5). M3: 단일 슬롯 + 콘솔 명령(ROHSave/ROHLoad).
 */
UCLASS()
class ROH_API UROHSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static const TCHAR* SlotName;

	/** 현재 플레이어 상태를 저장. 성공 시 true */
	bool SaveCharacter(AROHPlayerCharacter* Player);

	/**
	 * 저장된 상태를 불러와 적용. 저장 클래스가 다르면 해당 클래스로 재스폰한다.
	 * 반환: 적용된 플레이어 (실패 시 nullptr)
	 */
	AROHPlayerCharacter* LoadCharacter(AROHPlayerCharacter* CurrentPlayer);

	bool HasSave() const;
};
