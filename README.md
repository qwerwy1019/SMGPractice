# SMGPractice
[![게임 플레이 영상](http://img.youtube.com/vi/iUm7sT7fNdY/0.jpg)](https://youtu.be/iUm7sT7fNdY?t=0s)

https://youtu.be/iUm7sT7fNdY

Direct3D 12 사용. UI는 Direct2D, DirectWrite 사용.

다양한 기능을 개발할 수 있는 엔진을 목표로 두고 개발했습니다.


## SMGFileConverter
* .fbx 파일을 SMGEngine에서 읽는 xml형식의 데이터로 변환하는 프로그램
* Mesh, Animation, Material, Bone 정보를 추출합니다.
* 변환된 파일 형식은 SMGResources/XmlFiles/Asset 폴더에서 볼 수 있습니다.


## SMGEngine
* FileConverter로 나온 Asset과 직접 작성한 UI, ActionChart, StageInfo, ObjectInfo, StageScript를 사용하는 게임엔진

#### 충돌처리
* 액터는 Box(OBB), Sphere의 충돌 경계를 지원. Sector 범위 체크, AABB체크 후 충돌 체크를 시행합니다.
* 지형은 게임 특성상 폴리곤으로 충돌 체크. 연산을 줄이기 위해 지형 생성시 충돌 체크용 TerrainAABBNode를 생성합니다.
* 주요 내용은 MathHelper.h와 Terrain.h Actor::checkCollision에 있습니다.

#### D3D 관련
* 일반 액터에 비해 개수가 많은 이펙트 개체에만 인스턴싱 적용. 이펙트는 빌보드 방식으로 기하 쉐이더에서 만듭니다.
* d3d11On12 로 direct2D를 사용합니다.
* 그림자는 깊이 버퍼를 사용한 그림자 매핑으로 구현했습니다.
* skinning mesh animation으로 캐릭터 애니메이션을 구현했습니다.

#### 게임 로직 관련
* 마우스 포인터로 별을 먹고 쏘는 기능이 게임 전반에 있어서 프레임마다 raycast를 합니다.
* 게임 특성상 카메라가 다양한 각도를 비춰야 하기 때문에 주로 유저 입력이 아닌 자동 이동을 합니다. StageInfo의 카메라 데이터를 통해 동작합니다.
* 중력은 현재 구체 중심방향과 벡터(평면)이 지원됩니다.

## 실행 시
    https://github.com/qwerwy1019/SMGProjects/
    https://github.com/qwerwy1019/SMGResources/
    https://github.com/qwerwy1019/SMGPractice/
* Projects, Resources, Codes 폴더에 각각 있어야 합니다.
* FileConverter의 경우 fbx sdk가 필요합니다.
* Righteous 폰트가 필요합니다. (https://fonts.google.com/specimen/Righteous?query=right)

코드 내용에 대한 자세한 설명은 코드설명.md를 확인해주세요.
