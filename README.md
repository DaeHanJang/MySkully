# 💀 MySkully

> **프로젝트명**: MySkully
> 
> **장르**: 3D Adventure Platformer
>
> **개발 인원**: 1인
> 
> **개발 기간**: 2026.01 ~ 2026.02 (5주)
> 
> **개발 환경**: C++, Unreal Engine 5.6
> 
> **원작**: Skully
<img width="490" height="312" alt="Image" src="https://github.com/user-attachments/assets/eb582e50-e09d-4c3e-a16d-181244fd95c4" />

<br>

## 📖 프로젝트 소개

**MySkully**는 3D 어드벤처 플랫포머 게임 Skully를 모작한 개인 프로젝트입니다.

단순히 원작을 재현하는 것이 아니라, 구 형태 캐릭터에 적합한 이동 시스템을 직접 설계하는 것을 목표로 개발했습니다.
기본 CharacterMovementComponent 대신 UPawnMovementComponent를 기반으로 SkullyMovementComponent를 직접 구현하여 이동, 중력, 점프, 경사면 처리, 지면 판정 등을 설계했습니다.

<br>

## 🛠 기술 스택

<p>
<img src="https://img.shields.io/badge/C++-00599C?style=for-the-badge&logo=cplusplus&logoColor=white"/>
<img src="https://img.shields.io/badge/Unreal_Engine_5.6-313131?style=for-the-badge&logo=unrealengine&logoColor=white"/>
</p>

<br>

## ⭐ 세부 구현

### 🟠 [SkullyMovementComponent](https://github.com/DaeHanJang/MySkully/blob/main/Source/MySkully/Private/Skully/SkullyMovementComponent.cpp)

UPawnMovementComponent를 기반으로 구 형태 캐릭터에 적합한 이동 시스템을 직접 설계 및 구현했습니다.

#### 주요 기능
- 입력 기반 이동 처리
- 중력 및 점프 구현
- Grounded / Falling 상태 관리
- Ground Detection (Sphere Sweep + Line Trace)
- TargetVelocity 기반 가속/감속
- Stable / Unstable Ground 처리
- 경사면(Slope) 이동 및 Sliding
- Collision Resolution
- 실제 이동량 기반 Visual Roll 연출

### 🟠 Gameplay

플랫폼 게임 플레이를 위한 핵심 시스템을 구현했습니다.

#### [Player](https://github.com/DaeHanJang/MySkully/blob/main/Source/MySkully/Private/Skully/Skully.cpp) & [ClayMound](https://github.com/DaeHanJang/MySkully/blob/main/Source/MySkully/Private/Environment/ClayMound.cpp)
- Skully 플레이어 구현
- 힘 골렘 변신 시스템
- Save Point 및 HP 회복
- Collectible 시스템

#### [Enemy](https://github.com/DaeHanJang/MySkully/blob/main/Source/MySkully/Private/Enemy/WaterPunk.cpp)
- Enemy AI
- 플레이어 추적 및 전투
- Hazard 시스템

#### [UI](https://github.com/DaeHanJang/MySkully/blob/main/Source/MySkully/Private/SkullyHUDUserWidget.cpp)
- HUD
- 체력 UI
- 수집품 UI

#### [Environment](https://github.com/DaeHanJang/MySkully/blob/main/Source/MySkully/Private/Environment/DestructibleTile.cpp)
- 레벨 디자인
- 파괴 가능한 오브젝트
- 상호작용 오브젝트

<br>

## 📌 프로젝트 목표

- UPawnMovementComponent 기반의 커스텀 이동 시스템 설계
- 플랫폼 액션 게임에 필요한 이동 및 물리 시스템 구현
- C++ 중심의 Gameplay Programming 역량 강화

<br>

## 🔗 Links

- 🎥 [Video](https://youtu.be/97xa4fcjZwA)
- 📄 [Portfolio](https://drive.google.com/file/d/1frJZNKHnGa0OGk6YKVuEvys0Op7jI9vf/view?usp=sharing)
- 📚 [Notion](https://kind-rest-e61.notion.site/MySkully-2e53af988a68819fbd6fe4a57503fd19?source=copy_link)
