# SMGPractice
[![게임 플레이 영상](http://img.youtube.com/vi/iUm7sT7fNdY/0.jpg)](https://youtu.be/iUm7sT7fNdY?t=0s)


Direct3D 12 사용. UI는 Direct2D, DirectWrite 사용.

다양한 기능을 개발할 수 있는 엔진을 목표로 두고 개발했습니다.


## SMGFileConverter
* .fbx 파일을 SMGEngine에서 읽는 xml형식의 데이터로 변환하는 프로그램
* Mesh, Animation, Material, Bone 정보를 추출합니다.
* 변환된 파일 형식은 SMGResources/XmlFiles/Asset 폴더에서 볼 수 있습니다.


## SMGEngine
* FileConverter로 나온 Asset과 직접 작성한 UI, ActionChart, StageInfo, ObjectInfo, StageScript를 사용하는 게임엔진
### 충돌처리
